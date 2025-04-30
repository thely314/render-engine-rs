#include "Eigen/Core"
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "global.hpp"
#include <Scene.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
Scene::Scene(int width, int height)
    : eye_pos{0.0f, 0.0f, 0.0f}, view_dir{0.0f, 0.0f, -1.0f}, zNear(-0.1f),
      zFar(-1000.0f), width(width), height(height) {
  frame_buffer.resize(width * height);
  z_buffer.resize(width * height, -INFINITY);
}
void Scene::start_render() {

  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(eye_pos, view_dir),
                             projection =
                                 get_projection_matrix(90, 1.0f, -0.1, -100);
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  Eigen::Matrix<float, 3, 3> normal_mvp =
      view.block<3, 3>(0, 0).inverse().transpose() *
      model.block<3, 3>(0, 0).inverse().transpose();
  for (auto obj : objects) {
    obj->rasterization(mvp, normal_mvp, *this);
  }
}
void Scene::add_model(Object *model) { objects.push_back(model); }
void Scene::save_to_file(std::string filename) {
  std::vector<unsigned char> data(width * height * 3);
  for (int y = 0; y != height; ++y) {
    for (int x = 0; x != width; ++x) {
      data[3 * (y * width + x)] =
          std::clamp(frame_buffer[y * width + x].x(), 0.0f, 1.0f) * 255;
      data[3 * (y * width + x) + 1] =
          std::clamp(frame_buffer[y * width + x].y(), 0.0f, 1.0f) * 255;
      data[3 * (y * width + x) + 2] =
          std::clamp(frame_buffer[y * width + x].z(), 0.0f, 1.0f) * 255;
    }
  }
  stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}
Scene::~Scene() {
  for (auto obj : objects) {
    delete obj;
  }
}
int Scene::get_index(int x, int y) { return (width * (height - y - 1) + x); }
void Scene::set_eye_pos(const Eigen::Vector3f &eye_pos) {
  this->eye_pos = eye_pos;
}
void Scene::set_view_dir(const Eigen::Vector3f &view_dir) {
  this->view_dir = view_dir;
}
void Scene::set_zNear(float zNear) { this->zNear = zNear; }
void Scene::set_zFar(float zFar) { this->zFar = zFar; }