#include "Model.hpp"
#include "Scene.hpp"
#include "global.hpp"
#include "light.hpp"
#include <algorithm>
#include <cmath>
#include <functional>

constexpr int directional_light_sample_num = 64;
DirectionalLight::DirectionalLight()
    : light(), light_dir(0.0f, 0.0f, -1.0f), view_width(50.0f),
      view_height(50.0f), angular_diameter(3.0f), zNear(-0.1f), zFar(-1000.0f),
      pixel_radius(0.0f), zbuffer_width(8192), zbuffer_height(8192),
      penumbra_mask_width(0), penumbra_mask_height(0), enable_shadow(true),
      enable_pcf_sample_accelerate(true), enable_pcss_sample_accelerate(true),
      enable_penumbra_mask(true), mvp(Eigen::Matrix<float, 4, 4>::Identity()),
      mv(Eigen::Matrix<float, 4, 4>::Identity()) {}

Eigen::Vector3f DirectionalLight::get_light_dir() const { return light_dir; }

void DirectionalLight::set_light_dir(const Eigen::Vector3f dir) {
  light_dir = dir.normalized();
}

float DirectionalLight::get_angular_diameter() const {
  return angular_diameter;
}

void DirectionalLight::set_angular_diameter(float angular_diameter) {
  this->angular_diameter = angular_diameter;
}

float DirectionalLight::get_view_width() const { return view_width; }

void DirectionalLight::set_view_width(float view_width) {
  this->view_width = view_width;
}

float DirectionalLight::get_view_height() const { return view_height; }

void DirectionalLight::set_view_height(float view_height) {
  this->view_height = view_height;
}

float DirectionalLight::get_zNear() const { return zNear; }

void DirectionalLight::set_zNear(float zNear) { this->zNear = zNear; }

float DirectionalLight::get_zFar() const { return zFar; }

void DirectionalLight::set_zFar(float zFar) { this->zFar = zFar; }

int DirectionalLight::get_width() const { return zbuffer_width; }

void DirectionalLight::set_width(int width) { zbuffer_width = width; }

int DirectionalLight::get_height() const { return zbuffer_height; }

void DirectionalLight::set_height(int height) { zbuffer_height = height; }

bool DirectionalLight::get_shadow_status() const { return enable_shadow; }

void DirectionalLight::set_shadow_status(bool status) {
  enable_shadow = status;
}

bool DirectionalLight::get_pcf_sample_accelerate_status() const {
  return enable_pcf_sample_accelerate;
}

void DirectionalLight::set_pcf_sample_accelerate_status(bool status) {
  enable_pcf_sample_accelerate = status;
}

bool DirectionalLight::get_pcss_sample_accelerate_status() const {
  return enable_pcss_sample_accelerate;
}

void DirectionalLight::set_pcss_sample_accelerate_status(bool status) {
  enable_pcss_sample_accelerate = status;
}
bool DirectionalLight::get_penumbra_mask_status() const {
  return enable_penumbra_mask;
}
void DirectionalLight::set_penumbra_mask_status(bool status) {
  enable_penumbra_mask = status;
}

int DirectionalLight::get_index(int x, int y) const {
  return zbuffer_width * y + x;
}

int DirectionalLight::get_penumbra_mask_index(int x, int y) const {
  return penumbra_mask_width * y + x;
}

Eigen::Vector3f DirectionalLight::compute_world_light_dir(
    const Eigen::Vector3f point_pos) const {
  return light_dir;
}

Eigen::Vector3f DirectionalLight::compute_world_light_intensity(
    const Eigen::Vector3f point_pos) const {
  return intensity;
}

void DirectionalLight::look_at(const Scene &scene) {
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
  int zbuffer_width = this->zbuffer_width;
  int zbuffer_height = this->zbuffer_height;
  float z_near = zNear;
  float z_far = zFar;
  auto get_index_lambda = [zbuffer_width](int x, int y) {
    return zbuffer_width * y + x;
  };
  auto depth_transformer = [z_near, z_far](float z, float w) {
    return 0.5f * (z * (z_near - z_far) + (z_near + z_far));
  };
#pragma omp parallel for collapse(2)
  for (int j = 0; j < zbuffer_height; j += tile_size) {
    for (int i = 0; i < zbuffer_width; i += tile_size) {
      for (auto &&obj : scene.objects) {
        obj->rasterization_shadow_map_block<false>(
            z_buffer, j, i, std::min(tile_size, zbuffer_height - j),
            std::min(tile_size, zbuffer_width - i), depth_transformer,
            get_index_lambda);
      }
    }
  }
}
float DirectionalLight::in_shadow(Eigen::Vector3f point_pos,
                                  Eigen::Vector3f normal,
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

bool DirectionalLight::in_penumbra_mask(int x, int y) const {
  if (enable_shadow && enable_penumbra_mask) {
    return penumbra_mask[get_penumbra_mask_index(x / 4, y / 4)] > EPSILON;
  }
  return true;
}

float DirectionalLight::in_shadow_direct(const Eigen::Vector3f point_pos,
                                         const Eigen::Vector3f normal) const {
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
  int center_x = std::clamp(int((transform_pos.x() / transform_pos.w() + 1.0f) *
                                0.5f * zbuffer_width),
                            0, zbuffer_width - 1);
  int center_y = std::clamp(int((transform_pos.y() / transform_pos.w() + 1.0f) *
                                0.5f * zbuffer_height),
                            0, zbuffer_height - 1);
  transform_pos = mv * point_pos.homogeneous();
  float cosval = light_dir.dot(-normal) * light_dir.dot(-normal);
  const float bias =
      std::max(0.05f, sqrtf((1 - cosval) / (cosval)) * sqrtf(2) * pixel_radius);
  if (transform_pos.z() + bias > z_buffer[get_index(center_x, center_y)]) {
    return 1.0f;
  }
  return 0.0f;
}

float DirectionalLight::in_shadow_pcf(const Eigen::Vector3f point_pos,
                                      const Eigen::Vector3f normal) const {
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

  int unshadow_num = 0;
  int center_x = std::clamp(int((transform_pos.x() / transform_pos.w() + 1.0f) *
                                0.5f * zbuffer_width),
                            0, zbuffer_width - 1);
  int center_y = std::clamp(int((transform_pos.y() / transform_pos.w() + 1.0f) *
                                0.5f * zbuffer_height),
                            0, zbuffer_height - 1);
  transform_pos = mv * point_pos.homogeneous();
  float cosval = light_dir.dot(-normal) * light_dir.dot(-normal);
  const float bias =
      std::max(0.05f, sqrtf((1 - cosval) / (cosval)) * sqrtf(2) * pixel_radius);
  constexpr int pcf_radius = 1;
  if (pcf_radius < 6 || !enable_pcf_sample_accelerate) {
    for (int y = -pcf_radius; y <= pcf_radius; ++y) {
      for (int x = -pcf_radius; x <= pcf_radius; ++x) {
        int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
        int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
        if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
            z_buffer[get_index(idx_x, idx_y)]) {
          ++unshadow_num;
        }
      }
    }
    return unshadow_num * 1.0f / directional_light_sample_num;
  } else {
    const float sample_num_inverse = 1.0f / directional_light_sample_num;
    for (int i = 0; i < directional_light_sample_num; ++i) {
      Eigen::Vector2f sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
          i, sample_num_inverse, fibonacci_clump_exponent, 0.0f);
      int x = roundf(pcf_radius * sample_dir.x()),
          y = roundf(pcf_radius * sample_dir.y());
      int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
      int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
      if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
          z_buffer[get_index(idx_x, idx_y)]) {
        ++unshadow_num;
      }
    }
    return unshadow_num * 1.0f / ((2 * pcf_radius + 1) * (2 * pcf_radius + 1));
  }
}

float DirectionalLight::in_shadow_pcss(const Eigen::Vector3f point_pos,
                                       const Eigen::Vector3f normal) const {
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
  int center_x = std::clamp(int((transform_pos.x() / transform_pos.w() + 1.0f) *
                                0.5f * zbuffer_width),
                            0, zbuffer_width - 1);
  int center_y = std::clamp(int((transform_pos.y() / transform_pos.w() + 1.0f) *
                                0.5f * zbuffer_height),
                            0, zbuffer_height - 1);
  transform_pos = mv * point_pos.homogeneous();
  float cosval = light_dir.dot(-normal) * light_dir.dot(-normal);
  const float bias =
      std::max(0.05f, sqrtf((1 - cosval) / (cosval)) * sqrtf(2) * pixel_radius);
  float light_size_div_distance = 2.0f * tan(angular_diameter / 360.0f * M_PI);
  int pcss_radius =
      std::max(1.0f, 2.5f * light_size_div_distance / pixel_radius);
  // 魔数是试出来的
  // 从理想模型上看，它与zNear的大小有关，但是从实际上看又与zNear无关
  // pcss_radius越大，blocker搜索范围也就越大，一个像素的blocker_num不为0的概率也就越高
  // 所以pcss_radius决定了半影的面积，决定了有多少像素会参与到下面的pcf计算
  int block_num = 0;
  float block_depth = 0.0f;
  if (pcss_radius < 6 || !enable_pcss_sample_accelerate) {
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
    const float sample_num_inverse = 1.0f / directional_light_sample_num;
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
  // 0.25f * penumbra / pixel_radius == (0.5f * penumbra) / (2.0f *
  // pixel_radius)
  // pcf_radius决定了阴影的过渡速度，pcf_radius越小，过渡越迅速
  // 所谓过渡速度，是指不同像素之间阴影量的跳变程度
  // 在启用penumbra_mask之后，如果不调小pcf_radius，可能会导致半影面积不够过渡而引起的边缘突变
  // 我们认为按着正确的过渡速度,应该在一定的距离之后，半影才彻底弱化为无影
  // 但是penumbra_mask会砍半影面积，导致在给定距离内无法过渡到无影状态
  // 看上去就像影子在边缘很突兀的消失了，因为在边缘阴影还没弱到足够的程度
  // 解决方法是砍pcf_radius让阴影过渡的更快，但是这会导致阴影比真实的要硬
  int unshadow_num = 0;
  if (pcf_radius < 6 || !enable_pcf_sample_accelerate) {
    for (int y = -pcf_radius; y <= pcf_radius; ++y) {
      for (int x = -pcf_radius; x <= pcf_radius; ++x) {
        int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
        int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
        if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
            z_buffer[get_index(idx_x, idx_y)]) {
          ++unshadow_num;
        }
      }
    }
    return unshadow_num * 1.0f / ((2 * pcf_radius + 1) * (2 * pcf_radius + 1));
  } else {
    const float sample_num_inverse = 1.0f / directional_light_sample_num;
    for (int i = 0; i < directional_light_sample_num; ++i) {
      Eigen::Vector2f sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
          i, sample_num_inverse, fibonacci_clump_exponent, 0.0f);
      int x = roundf(pcf_radius * sample_dir.x()),
          y = roundf(pcf_radius * sample_dir.y());
      int idx_x = std::clamp(center_x + x, 0, zbuffer_width - 1);
      int idx_y = std::clamp(center_y + y, 0, zbuffer_height - 1);
      if (transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
          z_buffer[get_index(idx_x, idx_y)]) {
        ++unshadow_num;
      }
    }
    return unshadow_num * 1.0f / directional_light_sample_num;
  }
}
void DirectionalLight::generate_penumbra_mask(const Scene &scene) {
  if (!enable_shadow || !enable_penumbra_mask) {
    return;
  }
  penumbra_mask_width = ceilf(0.25f * scene.width);
  penumbra_mask_height = ceilf(0.25f * scene.height);
  penumbra_mask.resize(penumbra_mask_width * penumbra_mask_height);
  std::fill(penumbra_mask.begin(), penumbra_mask.end(), 0.0f);
#pragma omp parallel for collapse(2) schedule(static)
  for (int j = 0; j < penumbra_mask_height; j += tile_size) {
    for (int i = 0; i < penumbra_mask_width; i += tile_size) {
      generate_penumbra_mask_block(
          scene, j, i, std::min(tile_size, penumbra_mask_height - j),
          std::min(tile_size, penumbra_mask_width - i));
    }
  }
}
void DirectionalLight::generate_penumbra_mask_block(const Scene &scene,
                                                    int start_row,
                                                    int start_col,
                                                    int block_row,
                                                    int block_col) {
  for (int y = start_row; y < start_row + block_row; ++y) {
    for (int x = start_col; x < start_col + block_col; ++x) {
      int start_x = 4 * x, start_y = 4 * y;
      int pcf_num = 0, unshadow_num = 0;
      for (int v = start_y; v < start_y + 4; ++v) {
        for (int u = start_x; u < start_x + 4; ++u) {
          float clamp_u = std::clamp(u, 0, scene.width - 1);
          float clamp_v = std::clamp(v, 0, scene.height - 1);
          int idx = scene.get_index(clamp_u, clamp_v);
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
      if (unshadow_num == 0 || pcf_num == unshadow_num) {
        penumbra_mask[idx] = 0.0f;
      } else {
        penumbra_mask[idx] = 1.0f;
      }
    }
  }
}
void DirectionalLight::box_blur_penumbra_mask(int radius) {
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