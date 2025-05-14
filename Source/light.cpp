#include "light.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <thread>
constexpr float bias_scale = 0.04f; // 经验值
constexpr int sample_num = 16;
constexpr float poisson_disk[16][2] = {
    {-0.94201624, -0.39906216},  {0.94558609, -0.76890725},
    {-0.094184101, -0.92938870}, {0.34495938, 0.29387760},
    {-0.91588581, 0.45771432},   {-0.81544232, -0.87912464},
    {-0.38277543, 0.27676845},   {0.97484398, 0.75648379},
    {0.44323325, -0.97511554},   {0.53742981, -0.47373420},
    {-0.26496911, -0.41893023},  {0.79197514, 0.19090188},
    {-0.24188840, 0.99706507},   {-0.81409955, 0.91437590},
    {0.19984126, 0.78641367},    {0.14383161, -0.14100790}};
light::light() : pos(0.0f, 0.0f, 0.0f), intensity(0.0f, 0.0f, 0.0f) {}
light::light(const Eigen::Vector3f &pos, const Eigen::Vector3f &intensity)
    : pos(pos), intensity(intensity) {}

Eigen::Vector3f light::get_pos() const { return pos; }

void light::set_pos(const Eigen::Vector3f &pos) { this->pos = pos; }

Eigen::Vector3f light::get_intensity() const { return intensity; }

void light::set_intensity(const Eigen::Vector3f &intensity) {
  this->intensity = intensity;
}

void light::look_at(const Scene &scene) {}

bool light::in_shadow(Vertex_rasterization &point) { return false; }
float light::in_shadow_pcf(Vertex_rasterization &point) { return 1.0f; }
float light::in_shadow_pcss(Vertex_rasterization &point) { return 1.0f; }
spot_light::spot_light()
    : light(), light_dir(0.0f, 0.0f, -1.0f), fov(90.0f), aspect_ratio(1.0f),
      zNear(-0.1f), zFar(-1000.0f), light_size(1.0f), fov_factor(0.0f),
      zbuffer_width(2048), zbuffer_height(2048), enable_shadow(true),
      enable_pcf_poisson(true), enable_pcss_poisson(true),
      mvp(Eigen::Matrix<float, 4, 4>::Identity()) {
  z_buffer.resize(zbuffer_width * zbuffer_height, -INFINITY);
}

Eigen::Vector3f spot_light::get_light_dir() const { return light_dir; }

void spot_light::set_light_dir(const Eigen::Vector3f &dir) { light_dir = dir; }

float spot_light::get_fov() const { return fov; }

void spot_light::set_fov(float fov) { this->fov = fov; }

float spot_light::get_aspect_ratio() const { return aspect_ratio; }

void spot_light::set_aspect_ratio(float aspect_ratio) {
  this->aspect_ratio = aspect_ratio;
}

float spot_light::get_zNear() const { return zNear; }

void spot_light::set_zNear(float zNear) { this->zNear = zNear; }

float spot_light::get_zFar() const { return zFar; }

void spot_light::set_zFar(float zFar) { this->zFar = zFar; }

int spot_light::get_width() const { return zbuffer_width; }

void spot_light::set_width(int width) { zbuffer_width = width; }

int spot_light::get_height() const { return zbuffer_height; }

void spot_light::set_height(int height) { zbuffer_height = height; }

int spot_light::get_index(int x, int y) {
  return zbuffer_width * (zbuffer_height - y - 1) + x;
}

bool spot_light::get_shadow_status() const { return enable_shadow; }

void spot_light::set_shadow_status(bool status) { enable_shadow = status; }

bool spot_light::get_pcf_poisson_status() const { return enable_pcf_poisson; }

void spot_light::set_pcf_poisson_status(bool status) {
  enable_pcf_poisson = status;
}

bool spot_light::get_pcss_poisson_status() const { return enable_pcss_poisson; }

void spot_light::set_pcss_poisson_status(bool status) {
  enable_pcss_poisson = status;
}

void spot_light::look_at(const Scene &scene) {
  if (!enable_shadow) {
    return;
  }
  std::fill(z_buffer.begin(), z_buffer.end(), -INFINITY);
  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(pos, light_dir),
                             projection = get_projection_matrix(
                                 fov, aspect_ratio, zNear, zFar);
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  this->mvp = mvp;
  this->mv = view * model;
  constexpr float to_radian = M_PI / 360.0f;
  fov_factor = tan(to_radian * fov);
  for (auto obj : scene.objects) {
    obj->clip(mvp, mv);
  }
  for (auto obj : scene.objects) {
    obj->to_NDC(zbuffer_width, zbuffer_height);
  }
  int thread_num = std::min(zbuffer_width, maximum_thread_num);
  int thread_render_row_num = ceil(zbuffer_width * 1.0 / maximum_thread_num);
  std::vector<std::thread> threads;
  auto render_lambda = [](const Scene &scene, spot_light &light,
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

bool spot_light::in_shadow(Vertex_rasterization &point) {
  if (!enable_shadow) {
    return 1.0f;
  }
  point.transform_pos = point.pos.homogeneous();
  point.transform_pos = mvp * point.transform_pos;
  if (point.transform_pos.x() < point.transform_pos.w() ||
      point.transform_pos.x() > -point.transform_pos.w() ||
      point.transform_pos.y() < point.transform_pos.w() ||
      point.transform_pos.y() > -point.transform_pos.w() ||
      point.transform_pos.z() < point.transform_pos.w() ||
      point.transform_pos.z() > -point.transform_pos.w()) {
    return true;
  }
  point.transform_pos.x() /= point.transform_pos.w();
  point.transform_pos.y() /= point.transform_pos.w();
  point.transform_pos.x() =
      (point.transform_pos.x() + 1) * 0.5f * zbuffer_width;
  point.transform_pos.y() =
      (point.transform_pos.y() + 1) * 0.5f * zbuffer_height;
  int x_to_int = std::clamp((int)point.transform_pos.x(), 0, zbuffer_width - 1);
  int y_to_int =
      std::clamp((int)point.transform_pos.y(), 0, zbuffer_height - 1);
  point.transform_pos = mv * point.pos.homogeneous();
  float bias =
      std::max(0.2f,
               1.0f *
                   (1.0f - (pos - point.pos).normalized().dot(point.normal))) *
      EPSILON * bias_scale * zbuffer_height * fov_factor *
      -point.transform_pos.z();
  if (point.transform_pos.z() + bias >
      z_buffer[get_index(x_to_int, y_to_int)]) {
    return false;
  }
  return true;
}
float spot_light::in_shadow_pcf(Vertex_rasterization &point) {
  if (!enable_shadow) {
    return 1.0f;
  }
  point.transform_pos = point.pos.homogeneous();
  point.transform_pos = mvp * point.transform_pos;
  if (point.transform_pos.x() < point.transform_pos.w() ||
      point.transform_pos.x() > -point.transform_pos.w() ||
      point.transform_pos.y() < point.transform_pos.w() ||
      point.transform_pos.y() > -point.transform_pos.w() ||
      point.transform_pos.z() < point.transform_pos.w() ||
      point.transform_pos.z() > -point.transform_pos.w()) {
    return 0.0f;
  }
  point.transform_pos.x() /= point.transform_pos.w();
  point.transform_pos.y() /= point.transform_pos.w();
  point.transform_pos.x() =
      (point.transform_pos.x() + 1) * 0.5f * zbuffer_width;
  point.transform_pos.y() =
      (point.transform_pos.y() + 1) * 0.5f * zbuffer_height;
  int pcf_num = 0, unshadow_num = 0;
  int center_x = point.transform_pos.x(), center_y = point.transform_pos.y();
  point.transform_pos = mv * point.pos.homogeneous();
  float bias =
      std::max(0.2f,
               1.0f *
                   (1.0f - (pos - point.pos).normalized().dot(point.normal))) *
      EPSILON * bias_scale * zbuffer_height * fov_factor *
      -point.transform_pos.z();
  constexpr int pcf_radius = 1;
  if (pcf_radius < 2 || !enable_pcf_poisson) {
    for (int y = -pcf_radius; y <= pcf_radius; ++y) {
      if (center_y + y >= 0 && center_y + y < zbuffer_height) {
        for (int x = -pcf_radius; x <= pcf_radius; ++x) {
          if (center_x + x >= 0 && center_x + x < zbuffer_width) {
            ++pcf_num;
            if (point.transform_pos.z() +
                    (std::max(abs(x), abs(y)) + 1) * bias >
                z_buffer[get_index(center_x + x, center_y + y)]) {
              ++unshadow_num;
            }
          }
        }
      }
    }
  } else {
    for (int i = 0; i < sample_num; ++i) {
      int x = roundf(pcf_radius * poisson_disk[i][0]),
          y = roundf(pcf_radius * poisson_disk[i][1]);
      if (center_y + y >= 0 && center_y + y < zbuffer_height &&
          center_x + x >= 0 && center_x + x < zbuffer_width) {
        ++pcf_num;
        if (point.transform_pos.z() - (std::max(abs(x), abs(y)) + 1) * bias <
            z_buffer[get_index(center_x + x, center_y + y)]) {
          ++unshadow_num;
        }
      }
    }
  }
  return unshadow_num * 1.0f / pcf_num;
}
float spot_light::in_shadow_pcss(Vertex_rasterization &point) {
  if (!enable_shadow) {
    return 1.0f;
  }
  point.transform_pos = mvp * point.pos.homogeneous();
  if (point.transform_pos.x() < point.transform_pos.w() ||
      point.transform_pos.x() > -point.transform_pos.w() ||
      point.transform_pos.y() < point.transform_pos.w() ||
      point.transform_pos.y() > -point.transform_pos.w() ||
      point.transform_pos.z() < point.transform_pos.w() ||
      point.transform_pos.z() > -point.transform_pos.w()) {
    return 1.0f;
  }
  point.transform_pos.x() /= point.transform_pos.w();
  point.transform_pos.y() /= point.transform_pos.w();
  point.transform_pos.x() =
      (point.transform_pos.x() + 1) * 0.5f * zbuffer_width;
  point.transform_pos.y() =
      (point.transform_pos.y() + 1) * 0.5f * zbuffer_height;
  int center_x = point.transform_pos.x(), center_y = point.transform_pos.y();
  point.transform_pos = mv * point.pos.homogeneous();
  float bias =
      std::max(0.2f,
               1.0f *
                   (1.0f - (pos - point.pos).normalized().dot(point.normal))) *
      EPSILON * bias_scale * zbuffer_height * fov_factor *
      -point.transform_pos.z();
  int pcss_radius = std::max(
      1.0f, roundf((point.transform_pos.z() + 1) / point.transform_pos.z() *
                   light_size / fov_factor * zbuffer_height / 128.0f));
  int block_num = 0;
  float block_depth = 0.0f;
  if (pcss_radius < 2 || !enable_pcss_poisson) {
    for (int y = -pcss_radius; y <= pcss_radius; ++y) {
      if (center_y + y >= 0 && center_y + y < zbuffer_height) {
        for (int x = -pcss_radius; x <= pcss_radius; ++x) {
          if (center_x + x >= 0 && center_x + x < zbuffer_width) {
            if (point.transform_pos.z() +
                    (std::max(abs(x), abs(y)) + 1) * bias <
                z_buffer[get_index(center_x + x, center_y + y)]) {
              block_depth += z_buffer[get_index(center_x + x, center_y + y)];
              ++block_num;
            };
          }
        }
      }
    }
  } else {
    for (int i = 0; i < sample_num; ++i) {
      int x = roundf(pcss_radius * poisson_disk[i][0]),
          y = roundf(pcss_radius * poisson_disk[i][1]);
      if (center_y + y >= 0 && center_y + y < zbuffer_height &&
          center_x + x >= 0 && center_x + x < zbuffer_width) {
        if (point.transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias <
            z_buffer[get_index(center_x + x, center_y + y)]) {
          block_depth += z_buffer[get_index(center_x + x, center_y + y)];
          ++block_num;
        }
      }
    }
  }
  if (block_num == 0 || block_depth > -EPSILON) {
    return 1.0f;
  }
  block_depth /= block_num;
  float penumbra =
      (point.transform_pos.z() - block_depth) / block_depth * light_size;
  int pcf_radius =
      std::max(0.0f, roundf(penumbra * 0.5f * zbuffer_width /
                            (-point.transform_pos.z() * fov_factor)));
  pcf_radius = std::max(1, pcf_radius);
  // pcf_radius = 0;
  int pcf_num = 0, unshadow_num = 0;
  if (pcf_radius < 2 || !enable_pcf_poisson) {
    for (int y = -pcf_radius; y <= pcf_radius; ++y) {
      if (center_y + y >= 0 && center_y + y < zbuffer_height) {
        for (int x = -pcf_radius; x <= pcf_radius; ++x) {
          if (center_x + x >= 0 && center_x + x < zbuffer_width) {
            ++pcf_num;
            if (point.transform_pos.z() +
                    (std::max(abs(x), abs(y)) + 1) * bias >
                z_buffer[get_index(center_x + x, center_y + y)]) {
              ++unshadow_num;
            }
          }
        }
      }
    }
  } else {
    for (int i = 0; i < sample_num; ++i) {
      int x = roundf(pcf_radius * poisson_disk[i][0]),
          y = roundf(pcf_radius * poisson_disk[i][1]);
      if (center_y + y >= 0 && center_y + y < zbuffer_height &&
          center_x + x >= 0 && center_x + x < zbuffer_width) {
        ++pcf_num;
        if (point.transform_pos.z() + (std::max(abs(x), abs(y)) + 1) * bias >
            z_buffer[get_index(center_x + x, center_y + y)]) {
          ++unshadow_num;
        }
      }
    }
  }
  return unshadow_num * 1.0f / pcf_num;
}