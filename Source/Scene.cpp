#include "Eigen/Core"
#include "Model.hpp"
#include "global.hpp"
#include "light.hpp"
#include <Scene.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

Scene::Scene(int width, int height)
    : eye_pos{0.0f, 0.0f, 0.0f}, view_dir{0.0f, 0.0f, -1.0f}, fov(45.0f),
      aspect_ratio(1.0f), zNear(-0.1f), zFar(-1000.0f), width(width),
      height(height) {}

void Scene::start_render() {
  frame_buffer.resize(width * height, {0.7f, 0.7f, 0.7f});
  pos_buffer.resize(width * height, {0.0f, 0.0f, 0.0f});
  normal_buffer.resize(width * height, {0.0f, 0.0f, 0.0f});
  diffuse_buffer.resize(width * height, {0.0f, 0.0f, 0.0f});
  specular_buffer.resize(width * height, {0.0f, 0.0f, 0.0f});
  glow_buffer.resize(width * height, {0.0f, 0.0f, 0.0f});
  z_buffer.resize(width * height, INFINITY);
  std::fill(frame_buffer.begin(), frame_buffer.end(),
            Eigen::Vector3f{0.7f, 0.7f, 0.7f});
  std::fill(z_buffer.begin(), z_buffer.end(), INFINITY);
  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(eye_pos, view_dir),
                             projection = get_projection_matrix(
                                 fov, aspect_ratio, zNear, zFar);
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  Eigen::Matrix<float, 4, 4> mv = view * model;
  for (auto &&light : lights) {
    light->look_at(*this);
  }
  for (auto &&obj : objects) {
    obj->clip(mvp, mv);
    obj->to_NDC(width, height);
  }
#pragma omp parallel for collapse(2)
  for (int j = 0; j < height; j += tile_size) {
    for (int i = 0; i < width; i += tile_size) {
      for (auto &&obj : objects) {
        obj->rasterization_block(*this, j, i, std::min(tile_size, height - j),
                                 std::min(tile_size, width - i));
      }
    }
  }
  int box_radius = roundf(4.0f * std::max(width, height) / 1024.0f);
  for (auto &&light : lights) {
    light->generate_penumbra_mask(*this);
    light->box_blur_penumbra_mask(box_radius);
  }
#pragma omp parallel for collapse(2)
  for (int j = 0; j < height; j += tile_size) {
    for (int i = 0; i < width; i += tile_size) {
      for (auto &&obj : objects) {
        shader(*this, j, i, std::min(tile_size, height - j),
               std::min(tile_size, width - i));
      }
    }
  }
}

void Scene::add_model(const std::shared_ptr<Model> &model) {
  objects.push_back(model);
}

void Scene::add_light(const std::shared_ptr<light> &light) {
  lights.push_back(light);
}

void Scene::delete_model(const std::shared_ptr<Model> &model) {
  objects.erase(std::remove(objects.begin(), objects.end(), model),
                objects.end());
}
void Scene::delete_light(const std::shared_ptr<light> &light) {
  lights.erase(std::remove(lights.begin(), lights.end(), light), lights.end());
}

void Scene::save_to_file(std::string filename) {
  std::vector<unsigned char> data(width * height * 3);
  for (int y = 0; y != height; ++y) {
    for (int x = 0; x != width; ++x) {
      for (int i = 0; i != 3; ++i) {
        data[3 * (y * width + x) + i] =
            roundf(std::clamp(frame_buffer[width * (height - y - 1) + x][i],
                              0.0f, 1.0f) *
                   255);
      }
    }
  }
  stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}

int Scene::get_index(int x, int y) const { return width * y + x; }

void Scene::set_eye_pos(const Eigen::Vector3f eye_pos) {
  this->eye_pos = eye_pos;
}

void Scene::set_view_dir(const Eigen::Vector3f view_dir) {
  this->view_dir = view_dir.normalized();
}
void Scene::set_fov(float fov) { this->fov = fov; }

void Scene::set_aspect_ratio(float aspect_ratio) {
  this->aspect_ratio = aspect_ratio;
}

void Scene::set_zNear(float zNear) { this->zNear = zNear; }

void Scene::set_zFar(float zFar) { this->zFar = zFar; }

void Scene::set_width(int width) { this->width = width; }

void Scene::set_height(int height) { this->height = height; }

void Scene::set_shader(
    const std::function<void(Scene &Scene, int start_row, int start_col,
                             int block_row, int block_col)> &shader) {
  this->shader = shader;
}

Eigen::Vector3f Scene::get_eye_pos() const { return eye_pos; }

Eigen::Vector3f Scene::get_view_dir() const { return view_dir; }

float Scene::get_fov() const { return fov; }

float Scene::get_aspect_ratio() const { return aspect_ratio; }

float Scene::get_zNear() const { return zNear; }

float Scene::get_zFar() const { return zFar; }

int Scene::get_width() const { return width; }

int Scene::get_height() const { return height; }
std::function<void(Scene &Scene, int start_row, int start_col, int block_row,
                   int block_col)>
Scene::get_shader() const {
  return shader;
}