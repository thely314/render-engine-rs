#include "Scene.hpp"
#include "global.hpp"
#include "light.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <thread>
constexpr float directional_light_bias_scale = 10.0f;
constexpr int directional_light_sample_num = 64;
directional_light::directional_light()
    : light(), light_dir(0.0f, 0.0f, -1.0f), view_width(50.0f),
      view_height(50.0f), angular_diameter(3.0f), zNear(-0.1f), zFar(-1000.0f),
      pixel_radius(0.0f), zbuffer_width(2048), zbuffer_height(2048),
      penumbra_mask_width(0), penumbra_mask_height(0), enable_shadow(true),
      enable_pcf_sample_accelerate(true), enable_pcss_sample_accelerate(true),
      enable_penumbra_mask(true), mvp(Eigen::Matrix<float, 4, 4>::Identity()),
      mv(Eigen::Matrix<float, 4, 4>::Identity()) {}

Eigen::Vector3f directional_light::get_light_dir() const { return light_dir; }

void directional_light::set_light_dir(const Eigen::Vector3f &dir) {
  light_dir = dir;
}

float directional_light::get_view_width() const { return view_width; }

void directional_light::set_view_width(float view_width) {
  this->view_width = view_width;
}

float directional_light::get_view_height() const { return view_height; }

void directional_light::set_view_height(float view_height) {
  this->view_height = view_height;
}

float directional_light::get_zNear() const { return zNear; }

void directional_light::set_zNear(float zNear) { this->zNear = zNear; }

float directional_light::get_zFar() const { return zFar; }

void directional_light::set_zFar(float zFar) { this->zFar = zFar; }

int directional_light::get_width() const { return zbuffer_width; }

void directional_light::set_width(int width) { zbuffer_width = width; }

int directional_light::get_height() const { return zbuffer_height; }

void directional_light::set_height(int height) { zbuffer_height = height; }

bool directional_light::get_shadow_status() const { return enable_shadow; }

void directional_light::set_shadow_status(bool status) {
  enable_shadow = status;
}

bool directional_light::get_pcf_sample_accelerate_status() const {
  return enable_pcf_sample_accelerate;
}

void directional_light::set_pcf_sample_accelerate_status(bool status) {
  enable_pcf_sample_accelerate = status;
}

bool directional_light::get_pcss_sample_accelerate_status() const {
  return enable_pcss_sample_accelerate;
}

void directional_light::set_pcss_sample_accelerate_status(bool status) {
  enable_pcss_sample_accelerate = status;
}
bool directional_light::get_penumbra_mask_status() const {
  return enable_penumbra_mask;
}
void directional_light::set_penumbra_mask_status(bool status) {
  enable_penumbra_mask = status;
}

int directional_light::get_index(int x, int y) const {
  return zbuffer_width * y + x;
}

int directional_light::get_penumbra_mask_index(int x, int y) const {
  return penumbra_mask_width * y + x;
}

void directional_light::look_at(const Scene &scene) {
  if (!enable_shadow) {
    return;
  }
  z_buffer.resize(zbuffer_width * zbuffer_height, -INFINITY);
  std::fill(z_buffer.begin(), z_buffer.end(), -INFINITY);
  if (enable_penumbra_mask) {
    penumbra_mask_width = ceilf(0.25f * scene.width);
    penumbra_mask_height = ceilf(0.25f * scene.height);
    penumbra_mask.resize(penumbra_mask_width * penumbra_mask_height);
    std::fill(penumbra_mask.begin(), penumbra_mask.end(), 0.0f);
  }
  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(pos, light_dir), projection;
  projection << 2.0f / view_width, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f / view_height,
      0.0f, 0.0f, 0.0f, 0.0f, 2.0f / (zNear - zFar),
      -(zNear + zFar) / (zNear - zFar), 0.0f, 0.0f, 0.0f, -1.0f;
  // 为了兼容老代码所以最后一项是-1
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  this->mvp = mvp;
  this->mv = view * model;
  pixel_radius =
      0.5f * std::max(view_width / zbuffer_width, view_height / zbuffer_height);
  for (auto obj : scene.objects) {
    obj->clip(mvp, mv);
  }
  for (auto obj : scene.objects) {
    obj->to_NDC(zbuffer_width, zbuffer_height);
  }
  int thread_num = std::min(zbuffer_width, maximum_thread_num);
  int thread_render_row_num = ceil(zbuffer_width * 1.0 / maximum_thread_num);
  std::vector<std::thread> threads;
  auto render_lambda = [](const Scene &scene, directional_light &light,
                          int start_row, int block_row) {
    for (auto obj : scene.objects) {
      obj->rasterization_shadow_map_block(light, start_row, 0, block_row,
                                          light.zbuffer_width);
    }
  };
  for (int i = 0; i < thread_num - 1; ++i) {
    threads.emplace_back(render_lambda, std::ref(scene), std::ref(*this),
                         thread_render_row_num * i, thread_render_row_num);
  }
  threads.emplace_back(render_lambda, std::ref(scene), std::ref(*this),
                       thread_render_row_num * (thread_num - 1),
                       zbuffer_height -
                           thread_render_row_num * (thread_num - 1));
  for (auto &&thread : threads) {
    thread.join();
  }
}
float directional_light::in_shadow(const Eigen::Vector3f &point_pos,
                                   const Eigen::Vector3f &normal,
                                   SHADOW_METHOD shadow_method) const {
  switch (shadow_method) {
  case light::DIRECT:
    return in_shadow_direct(point_pos, normal);
  case light::PCF:
    return in_shadow_pcf(point_pos, normal);
  case light::PCSS:
    return in_shadow_pcss(point_pos, normal);
  }
  return 1.0f;
}

bool directional_light::in_penumbra_mask(int x, int y) {
  if (enable_shadow && enable_penumbra_mask) {
    return penumbra_mask[get_penumbra_mask_index(x * 0.25f, y * 0.25f)] >
           EPSILON;
  }
  return true;
}

float directional_light::in_shadow_direct(const Eigen::Vector3f &point_pos,
                                          const Eigen::Vector3f &normal) const {
  if (!enable_shadow) {
    return 1.0f;
  }
  Eigen::Vector4f transform_pos = mvp * point_pos.homogeneous();
  if (transform_pos.x() < transform_pos.w() ||
      transform_pos.x() > -transform_pos.w() ||
      transform_pos.y() < transform_pos.w() ||
      transform_pos.y() > -transform_pos.w() ||
      transform_pos.z() < transform_pos.w() ||
      transform_pos.z() > -transform_pos.w()) {
    return 0.0f;
  }
  transform_pos.x() /= transform_pos.w();
  transform_pos.y() /= transform_pos.w();
  transform_pos.x() = (transform_pos.x() + 1) * 0.5f * zbuffer_width;
  transform_pos.y() = (transform_pos.y() + 1) * 0.5f * zbuffer_height;
  int x_to_int = std::clamp((int)transform_pos.x(), 0, zbuffer_width - 1);
  int y_to_int = std::clamp((int)transform_pos.y(), 0, zbuffer_height - 1);
  transform_pos = mv * point_pos.homogeneous();
  float bias = directional_light_bias_scale *
               std::max(0.2f, 1.0f - light_dir.normalized().dot(-normal)) *
               pixel_radius;
  if (transform_pos.z() + bias > z_buffer[get_index(x_to_int, y_to_int)]) {
    return 1.0f;
  }
  return 0.0f;
}

float directional_light::in_shadow_pcf(const Eigen::Vector3f &point_pos,
                                       const Eigen::Vector3f &normal) const {
  if (!enable_shadow) {
    return 1.0f;
  }
  Eigen::Vector4f transform_pos = point_pos.homogeneous();
  transform_pos = mvp * transform_pos;
  if (transform_pos.x() < transform_pos.w() ||
      transform_pos.x() > -transform_pos.w() ||
      transform_pos.y() < transform_pos.w() ||
      transform_pos.y() > -transform_pos.w() ||
      transform_pos.z() < transform_pos.w() ||
      transform_pos.z() > -transform_pos.w()) {
    return 0.0f;
  }
  transform_pos.x() /= transform_pos.w();
  transform_pos.y() /= transform_pos.w();
  Eigen::Matrix<float, 2, 2> random_rotate =
      Eigen::Matrix<float, 2, 2>::Identity();
  transform_pos.x() = (transform_pos.x() + 1) * 0.5f * zbuffer_width;
  transform_pos.y() = (transform_pos.y() + 1) * 0.5f * zbuffer_height;
  int pcf_num = 0, unshadow_num = 0;
  int center_x = transform_pos.x(), center_y = transform_pos.y();
  transform_pos = mv * point_pos.homogeneous();
  float bias = directional_light_bias_scale *
               std::max(0.2f, 1.0f - light_dir.normalized().dot(-normal)) *
               pixel_radius;
  constexpr int pcf_radius = 1;
  if (pcf_radius < 2 || !enable_pcf_sample_accelerate) {
    for (int y = -pcf_radius; y <= pcf_radius; ++y) {
      for (int x = -pcf_radius; x <= pcf_radius; ++x) {
        int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
        int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
        ++pcf_num;
        if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
            z_buffer[get_index(idx_x, idx_y)]) {
          ++unshadow_num;
        }
      }
    }
  } else {
    float sample_num_inverse = 1.0f / directional_light_sample_num;
    for (int i = 0; i < directional_light_sample_num; ++i) {
      Eigen::Vector2f sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
          i, sample_num_inverse, fibonacci_clump_exponent, 0.0f);
      int x = roundf(pcf_radius * sample_dir.x()),
          y = roundf(pcf_radius * sample_dir.y());
      int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
      int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
      ++pcf_num;
      if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
          z_buffer[get_index(idx_x, idx_y)]) {
        ++unshadow_num;
      }
    }
  }
  return unshadow_num * 1.0f / pcf_num;
}

float directional_light::in_shadow_pcss(const Eigen::Vector3f &point_pos,
                                        const Eigen::Vector3f &normal) const {
  if (!enable_shadow) {
    return 1.0f;
  }
  Eigen::Vector4f transform_pos = mvp * point_pos.homogeneous();
  if (transform_pos.x() < transform_pos.w() ||
      transform_pos.x() > -transform_pos.w() ||
      transform_pos.y() < transform_pos.w() ||
      transform_pos.y() > -transform_pos.w() ||
      transform_pos.z() < transform_pos.w() ||
      transform_pos.z() > -transform_pos.w()) {
    return 1.0f;
  }
  transform_pos.x() /= transform_pos.w();
  transform_pos.y() /= transform_pos.w();
  Eigen::Matrix<float, 2, 2> random_rotate =
      Eigen::Matrix<float, 2, 2>::Identity();
  transform_pos.x() = (transform_pos.x() + 1) * 0.5f * zbuffer_width;
  transform_pos.y() = (transform_pos.y() + 1) * 0.5f * zbuffer_height;
  int center_x = transform_pos.x(), center_y = transform_pos.y();
  transform_pos = mv * point_pos.homogeneous();
  float bias = directional_light_bias_scale *
               std::max(0.2f, 1.0f - light_dir.normalized().dot(-normal)) *
               pixel_radius;
  float light_size_div_distance = 2.0f * tan(M_PI / 360.0f * angular_diameter);
  int pcss_radius =
      std::max(1.0f, 2.5f * light_size_div_distance / pixel_radius);
  // 魔数是试出来的
  // 从理想模型上看，它与zNear的大小有关，但是从实际上看又与zNear无关
  // pcss_radius越大，blocker搜索范围也就越大，一个像素的blocker_num不为0的概率也就越高
  // 所以pcss_radius决定了半影的面积，决定了有多少像素会参与到下面的pcf计算
  int block_num = 0;
  float block_depth = 0.0f;
  if (pcss_radius < 2 || !enable_pcss_sample_accelerate) {
    for (int y = -pcss_radius; y <= pcss_radius; ++y) {
      for (int x = -pcss_radius; x <= pcss_radius; ++x) {
        int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
        int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
        if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias <
            z_buffer[get_index(idx_x, idx_y)]) {
          block_depth += z_buffer[get_index(idx_x, idx_y)];
          ++block_num;
        };
      }
    }
  } else {
    float sample_num_inverse = 1.0f / directional_light_sample_num;
    for (int i = 0; i < directional_light_sample_num; ++i) {
      Eigen::Vector2f sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
          i, sample_num_inverse, fibonacci_clump_exponent, 0.0f);
      int x = roundf(pcss_radius * sample_dir.x()),
          y = roundf(pcss_radius * sample_dir.y());
      int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
      int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
      if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias <
          z_buffer[get_index(idx_x, idx_y)]) {
        block_depth += z_buffer[get_index(idx_x, idx_y)];
        ++block_num;
      }
    }
  }

  if (block_num == 0 || block_depth > -EPSILON) {
    return 1.0f;
  }
  block_depth /= block_num;
  float penumbra = (block_depth - transform_pos.z()) * light_size_div_distance;
  int pcf_radius = std::max(1.0f, roundf(0.25f * penumbra / pixel_radius));
  // pcf_radius决定了阴影的过渡速度，pcf_radius越小，过渡越迅速
  // 所谓过渡速度，是指不同像素之间阴影量的跳变程度
  // 在启用penumbra_mask之后，如果不调小pcf_radius，可能会导致半影面积不够过渡而引起的边缘突变
  // 我们认为按着正确的过渡速度,应该在一定的距离之后，半影才彻底弱化为无影
  // 但是penumbra_mask会砍半影面积，导致在给定距离内无法过渡到无影状态
  // 看上去就像影子在边缘很突兀的消失了，因为在边缘阴影还没弱到足够的程度
  // 解决方法是砍pcf_radius让阴影过渡的更快，但是这会导致阴影比真实的要硬
  int pcf_num = 0, unshadow_num = 0;
  if (pcf_radius < 2 || !enable_pcf_sample_accelerate) {
    for (int y = -pcf_radius; y <= pcf_radius; ++y) {
      for (int x = -pcf_radius; x <= pcf_radius; ++x) {
        int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
        int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
        ++pcf_num;
        if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
            z_buffer[get_index(idx_x, idx_y)]) {
          ++unshadow_num;
        }
      }
    }
  } else {
    float sample_num_inverse = 1.0f / directional_light_sample_num;
    for (int i = 0; i < directional_light_sample_num; ++i) {
      Eigen::Vector2f sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
          i, sample_num_inverse, fibonacci_clump_exponent, 0.0f);
      int x = roundf(pcf_radius * sample_dir.x()),
          y = roundf(pcf_radius * sample_dir.y());
      int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
      int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
      ++pcf_num;
      if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
          z_buffer[get_index(idx_x, idx_y)]) {
        ++unshadow_num;
      }
    }
  }
  return unshadow_num * 1.0f / pcf_num;
}
void directional_light::generate_penumbra_mask(const Scene &scene) {
  if (!enable_shadow || !enable_penumbra_mask) {
    return;
  }
  std::vector<std::thread> threads;
  int penumbra_width = ceilf(scene.width * 0.25f);
  int penumbra_height = ceilf(scene.height * 0.25f);
  int penumbra_mask_thread_num = std::min(penumbra_height, maximum_thread_num);
  int penumbra_mask_thread_row_num =
      ceilf(penumbra_height * 1.0f / penumbra_mask_thread_num);
  auto penumbra_mask_lambda = [](directional_light &light, const Scene &scene,
                                 int start_row, int block_row) {
    light.generate_penumbra_mask_block(scene, start_row, 0, block_row,
                                       ceilf(scene.width * 0.25f));
  };
  for (int i = 0; i < penumbra_mask_thread_num - 1; ++i) {
    threads.emplace_back(penumbra_mask_lambda, std::ref(*this), std::ref(scene),
                         penumbra_mask_thread_row_num * i,
                         penumbra_mask_thread_row_num);
  }
  threads.emplace_back(penumbra_mask_lambda, std::ref(*this), std::ref(scene),
                       penumbra_mask_thread_row_num *
                           (penumbra_mask_thread_num - 1),
                       penumbra_height - penumbra_mask_thread_row_num *
                                             (penumbra_mask_thread_num - 1));
  for (auto &&thread : threads) {
    thread.join();
  }
}
void directional_light::generate_penumbra_mask_block(const Scene &scene,
                                                     int start_row,
                                                     int start_col,
                                                     int block_row,
                                                     int block_col) {
  for (int y = start_row; y < start_row + block_row; ++y) {
    for (int x = start_col; x < start_col + block_col; ++x) {
      x = std::clamp(x, 0, penumbra_mask_width - 1);
      y = std::clamp(y, 0, penumbra_mask_height - 1);
      int start_x = 4 * x, start_y = 4 * y;
      int pcf_num = 0, unshadow_num = 0;
      for (int v = start_y; v < start_y + 4; ++v) {
        for (int u = start_x; u < start_x + 4; ++u) {
          u = std::clamp(u, 0, scene.width - 1);
          v = std::clamp(v, 0, scene.height - 1);
          int idx = scene.get_index(u, v);
          if (scene.z_buffer[idx] < INFINITY) {
            ++pcf_num;
            if (in_shadow_direct(scene.pos_buffer[idx],
                                 scene.normal_buffer[idx]) > EPSILON) {
              ++unshadow_num;
            }
          }
        }
      }
      int idx = get_penumbra_mask_index(x, y);
      if (pcf_num == 0 || unshadow_num == 0 || pcf_num == unshadow_num) {
        penumbra_mask[idx] = 0.0f;
      } else {
        penumbra_mask[idx] = 1.0f;
      }
    }
  }
}
void directional_light::box_blur_penumbra_mask(int radius) {
  if (!enable_shadow || !enable_penumbra_mask) {
    return;
  }
  int penumbra_mask_width = this->penumbra_mask_width;
  auto get_index_lambda = [penumbra_mask_width](int x, int y) {
    return penumbra_mask_width * y + x;
  };
  penumbra_mask = blur_penumbra_mask_vertical(
      blur_penumbra_mask_horizontal(penumbra_mask, penumbra_mask_width,
                                    penumbra_mask_height, radius,
                                    get_index_lambda),
      penumbra_mask_width, penumbra_mask_height, radius, get_index_lambda);
}