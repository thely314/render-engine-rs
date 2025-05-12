#include "light.hpp"
#include "Eigen/Core"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
constexpr float bias_scale = 0.2f; // 我放弃算了，直接调参吧
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
      zNear(-0.1f), zFar(-100.0f), light_size(5.0f), projection_scale(0.0f),
      pixel_radius(0.0f), zbuffer_width(2048), zbuffer_height(2048),
      mvp(Eigen::Matrix<float, 4, 4>::Identity()) {
  z_buffer.resize(zbuffer_width * zbuffer_height, INFINITY);
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

void spot_light::look_at(const Scene &scene) {
  std::fill(z_buffer.begin(), z_buffer.end(), INFINITY);
  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(pos, light_dir),
                             projection = get_projection_matrix(
                                 fov, aspect_ratio, zNear, zFar);
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  this->mvp = mvp;
  this->mv = view * model;
  constexpr float to_radian = M_PI / 360.0f;
  pixel_radius = -zNear * tan(to_radian * fov) / zbuffer_height;
  projection_scale = (zNear + zFar) / (zNear - zFar);
  this->normal_mv = view.block<3, 3>(0, 0).inverse().transpose() *
                    model.block<3, 3>(0, 0).inverse().transpose();
  for (auto obj : scene.objects) {
    obj->clip(mvp);
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
  float recv_z = point.transform_pos.z();
  Eigen::Vector3f transform_normal = (normal_mv * point.normal).normalized();
  Eigen::Vector3f center =
      Eigen::Vector3f{
          ((x_to_int - 0.5f * zbuffer_width) * 2.0f + 1) * pixel_radius,
          ((y_to_int - 0.5f * zbuffer_height) * 2.0f + 1) * pixel_radius, zNear}
          .normalized();
  Eigen::Vector3f edge =
      Eigen::Vector3f{((x_to_int - 0.5f * zbuffer_width) * 2.0f +
                       (transform_normal.x() < 0 ? 0.0f : 2.0f)) *
                          pixel_radius,
                      ((y_to_int - 0.5f * zbuffer_height) * 2.0f +
                       (transform_normal.y() < 0 ? 0.0f : 2.0f)) *
                          pixel_radius,
                      zNear}
          .normalized();
  float distance = (point.pos - pos).norm();
  float bias =
      -0.04f * 1024 / zbuffer_width +
      std::max(
          0.0f,
          ((center.dot(transform_normal) / edge.dot(transform_normal)) - 1.0f) *
              distance) *
          projection_scale;
  // std::cout << bias << '\n';
  // float bias =
  //     0.01f + bias_scale * 1024.0f / zbuffer_height *
  //                  (1.0f - (pos - point.pos).normalized().dot(point.normal));
  if (recv_z + bias < z_buffer[get_index(x_to_int, y_to_int)]) {
    return false;
  }
  return true;
}
float spot_light::in_shadow_pcf(Vertex_rasterization &point) {
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
  float bias =
      0.01f + bias_scale * 1024.0f / zbuffer_height *
                  (1.0f - (pos - point.pos).normalized().dot(point.normal));
  constexpr int pcf_radius = 3;
  for (int y = -pcf_radius; y <= pcf_radius; ++y) {
    if (center_y + y >= 0 && center_y + y < zbuffer_height) {
      for (int x = -pcf_radius; x <= pcf_radius; ++x) {
        if (center_x + x >= 0 && center_x + x < zbuffer_width) {
          ++pcf_num;
          if (point.transform_pos.z() - (pcf_radius + 1) * bias <
              z_buffer[get_index(center_x + x, center_y + y)]) {
            ++unshadow_num;
          }
        }
      }
    }
  }
  return unshadow_num * 1.0f / pcf_num;
}
float spot_light::in_shadow_pcss(Vertex_rasterization &point) {
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
  int center_x = point.transform_pos.x(), center_y = point.transform_pos.y();
  float point_projection_depth = point.transform_pos.z();
  point.transform_pos = mv * point.pos.homogeneous();
  float bias =
      0.01f + bias_scale * 1024.0f / zbuffer_height *
                  (1.0f - (pos - point.pos).normalized().dot(point.normal));
  // float light_frustum_width = ;
  // find findblocker radius
  constexpr int max_pcss_radius = 16, min_pcss_radius = 2;
  float s = 1.0f - (point.transform_pos.z() - zNear) / (zFar - zNear);
  int pcss_radius = min_pcss_radius + std::clamp(s, 0.0f, 1.0f) *
                                          (max_pcss_radius - min_pcss_radius);
  // find blocker
  int block_num = 0;
  float block_depth = 0.0f;
  for (int y = -pcss_radius; y <= pcss_radius; ++y) {
    if (center_y + y >= 0 && center_y + y < zbuffer_height) {
      for (int x = -pcss_radius; x <= pcss_radius; ++x) {
        if (center_x + x >= 0 && center_x + x < zbuffer_width) {
          if (point_projection_depth + (pcss_radius + 1) * bias >
              z_buffer[get_index(center_x + x, center_y + y)]) {
            block_depth += z_buffer[get_index(center_x + x, center_y + y)];
            ++block_num;
          };
        }
      }
    }
  }
  auto projection_to_view = [this](float projection_depth) -> float {
    return (projection_depth + 2 * zNear * zFar / (zNear - zFar)) *
           ((zNear - zFar) / (zNear + zFar));
  };
  block_depth /= block_num;
  block_depth = projection_to_view(block_depth);
  float penumbra =
      (point.transform_pos.z() - block_depth) / block_depth * light_size;
  // pcf
  // TODO: 把魔数16换成合理计算的参数
  int pcf_num = 0, unshadow_num = 0;
  constexpr float to_radian = M_PI / 360.0f;
  int pcf_radius = std::max(1, (int)(penumbra * 16));
  for (int y = -pcf_radius; y <= pcf_radius; ++y) {
    if (center_y + y >= 0 && center_y + y < zbuffer_height) {
      for (int x = -pcf_radius; x <= pcf_radius; ++x) {
        if (center_x + x >= 0 && center_x + x < zbuffer_width) {
          ++pcf_num;
          if (point_projection_depth - (pcf_radius + 1) * bias <
              z_buffer[get_index(center_x + x, center_y + y)]) {
            ++unshadow_num;
          }
        }
      }
    }
  }
  return unshadow_num * 1.0f / pcf_num;
}