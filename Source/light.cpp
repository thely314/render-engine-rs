#include "light.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include <cmath>
#include <cstdlib>
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

bool light::in_shadow(Vertex_rasterization &) { return false; }

spot_light::spot_light()
    : light(), light_dir(0.0f, 0.0f, -1.0f), fov(90.0f), aspect_ratio(1.0f),
      zNear(-0.1f), zFar(-100.0f), zbuffer_width(12800), zbuffer_height(12800),
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

void spot_light::look_at(const Scene &scene) {
  std::fill(z_buffer.begin(), z_buffer.end(), -INFINITY);
  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(pos, light_dir),
                             projection = get_projection_matrix(
                                 fov, aspect_ratio, zNear, zFar);
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  this->mvp = mvp;
  Eigen::Matrix<float, 3, 3> normal_mvp =
      view.block<3, 3>(0, 0).inverse().transpose() *
      model.block<3, 3>(0, 0).inverse().transpose();
  for (auto obj : scene.objects) {
    obj->clip(mvp);
  }
  for (auto obj : scene.objects) {
    obj->rasterization_shadow_map(mvp, *this);
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
  point.transform_pos.z() /= point.transform_pos.w();
  point.transform_pos.x() =
      (point.transform_pos.x() + 1) * 0.5f * zbuffer_width;
  point.transform_pos.y() =
      (point.transform_pos.y() + 1) * 0.5f * zbuffer_height;
  if (point.transform_pos.z() + EPSILON >
      z_buffer[get_index(point.transform_pos.x(), point.transform_pos.y())]) {
    return false;
  }
  return true;
}