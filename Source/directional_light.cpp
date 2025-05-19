#include "Scene.hpp"
#include "light.hpp"
#include <algorithm>
#include <thread>
constexpr float directional_light_bias_scale = 20.0f;
constexpr int directional_light_sample_num = 16;
directional_light::directional_light()
    : light(), light_dir(0.0f, 0.0f, -1.0f), view_width(100.0f),
      view_height(100.0f), angular_diameter(1.0f), zNear(-0.1f), zFar(-1000.0f),
      pixel_radius(0.0f), zbuffer_width(8192), zbuffer_height(8192),
      enable_shadow(true), enable_pcf_sample_accelerate(true),
      enable_pcss_sample_accelerate(true),
      mvp(Eigen::Matrix<float, 4, 4>::Identity()),
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

int directional_light::get_index(int x, int y) {
  return zbuffer_width * (zbuffer_height - y - 1) + x;
}

void directional_light::look_at(const Scene &scene) {
  if (!enable_shadow) {
    return;
  }
  z_buffer.resize(zbuffer_width * zbuffer_height, -INFINITY);
  std::fill(z_buffer.begin(), z_buffer.end(), -INFINITY);
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
                          const Eigen::Matrix<float, 4, 4> &mvp, int start_row,
                          int block_row) {
    for (auto obj : scene.objects) {
      obj->rasterization_shadow_map_block(mvp, light, start_row, 0, block_row,
                                          light.zbuffer_width);
    }
  };
  for (int i = 0; i < thread_num - 1; ++i) {
    threads.emplace_back(render_lambda, std::ref(scene), std::ref(*this),
                         std::ref(this->mvp), thread_render_row_num * i,
                         thread_render_row_num);
  }
  threads.emplace_back(
      render_lambda, std::ref(scene), std::ref(*this), std::ref(this->mvp),
      thread_render_row_num * (thread_num - 1),
      zbuffer_height - thread_render_row_num * (thread_num - 1));
  for (auto &&thread : threads) {
    thread.join();
  }
}

bool directional_light::in_shadow(const Eigen::Vector3f &point_pos,
                                  const Eigen::Vector3f &normal) {
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
    return true;
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
    return false;
  }
  return true;
}

float directional_light::in_shadow_pcf(const Eigen::Vector3f &point_pos,
                                       const Eigen::Vector3f &normal) {
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
                                        const Eigen::Vector3f &normal) {}

void directional_light::generate_penumbra_mask_block(
    const Scene &scene, std::vector<SHADOW_STATUS> &penumbra_mask,
    std::vector<float> &penumbra_mask_blur, int start_row, int start_col,
    int block_row, int block_col) {
  if (!enable_shadow) {
    return;
  }
  for (int y = start_row; y < start_row + block_row; ++y) {
    for (int x = start_col; x < start_col + block_col; ++x) {
      x = std::clamp(x, 0, scene.penumbra_mask_width - 1);
      y = std::clamp(y, 0, scene.penumbra_mask_height - 1);
      int start_x = 4 * x, start_y = 4 * y;
      int pcf_num = 0, unshadow_num = 0;
      // 进行4x4PCF运算
      for (int v = start_y; v < start_y + 4; ++v) {
        for (int u = start_x; u < start_x + 4; ++u) {
          u = std::clamp(u, 0, scene.width - 1);
          v = std::clamp(v, 0, scene.height - 1);
          int idx = scene.get_index(u, v);
          if (scene.z_buffer[idx] < INFINITY) {
            ++pcf_num;
            Eigen::Vector3f point_pos = scene.pos_buffer[idx];
            Eigen::Vector3f normal = scene.normal_buffer[idx];
            Eigen::Vector4f transform_pos = mvp * point_pos.homogeneous();
            if (transform_pos.x() < transform_pos.w() ||
                transform_pos.x() > -transform_pos.w() ||
                transform_pos.y() < transform_pos.w() ||
                transform_pos.y() > -transform_pos.w() ||
                transform_pos.z() < transform_pos.w() ||
                transform_pos.z() > -transform_pos.w()) {
              continue;
            }
            transform_pos.x() /= transform_pos.w();
            transform_pos.y() /= transform_pos.w();
            transform_pos.x() = (transform_pos.x() + 1) * 0.5f * zbuffer_width;
            transform_pos.y() = (transform_pos.y() + 1) * 0.5f * zbuffer_height;
            int x_to_int =
                std::clamp((int)transform_pos.x(), 0, zbuffer_width - 1);
            int y_to_int =
                std::clamp((int)transform_pos.y(), 0, zbuffer_height - 1);
            transform_pos = mv * point_pos.homogeneous();
            float bias =
                directional_light_bias_scale *
                std::max(0.2f,
                         1.0f * (1.0f - light_dir.normalized().dot(-normal))) *
                pixel_radius;
            if (transform_pos.z() + bias >
                z_buffer[get_index(x_to_int, y_to_int)]) {
              ++unshadow_num;
            }
          }
        }
      }
      int idx = scene.get_penumbra_mask_index(x, y);
      if (pcf_num == 0 || unshadow_num == 0) {
        penumbra_mask[idx] = SHADOW;
      } else if (pcf_num == unshadow_num) {
        penumbra_mask[idx] = BRIGHT;
      } else {
        penumbra_mask[idx] = PENUMBRA;
        penumbra_mask_blur[idx] = 1.0f;
      }
    }
  }
}