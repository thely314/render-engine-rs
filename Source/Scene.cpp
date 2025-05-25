#include "Eigen/Core"
#include "Model.hpp"
#include "global.hpp"
#include "light.hpp"
#include <Scene.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <thread>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

Scene::Scene(int width, int height)
    : eye_pos{0.0f, 0.0f, 0.0f}, view_dir{0.0f, 0.0f, -1.0f}, zNear(-0.1f),
      zFar(-1000.0f), width(width), height(height) {}

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
  std::fill(pos_buffer.begin(), pos_buffer.end(),
            Eigen::Vector3f{0.0f, 0.0f, 0.0f});
  std::fill(normal_buffer.begin(), normal_buffer.end(),
            Eigen::Vector3f{0.0f, 0.0f, 0.0f});
  std::fill(diffuse_buffer.begin(), diffuse_buffer.end(),
            Eigen::Vector3f{0.0f, 0.0f, 0.0f});
  std::fill(specular_buffer.begin(), specular_buffer.end(),
            Eigen::Vector3f{0.0f, 0.0f, 0.0f});
  std::fill(glow_buffer.begin(), glow_buffer.end(),
            Eigen::Vector3f{0.0f, 0.0f, 0.0f});
  std::fill(z_buffer.begin(), z_buffer.end(), INFINITY);
  Eigen::Matrix<float, 4, 4> model = Eigen::Matrix<float, 4, 4>::Identity(),
                             view = get_view_matrix(eye_pos, view_dir),
                             projection =
                                 get_projection_matrix(45, 1.0f, zNear, zFar);
  Eigen::Matrix<float, 4, 4> mvp = projection * view * model;
  Eigen::Matrix<float, 4, 4> mv = view * model;
  for (auto &&light : lights) {
    light->look_at(*this);
  }
  for (auto &&obj : objects) {
    obj->clip(mvp, mv);
    obj->to_NDC(width, height);
  }
  int thread_num = std::min(height, maximum_thread_num);
  int thread_render_row_num = ceilf(height * 1.0f / thread_num);
  std::vector<std::thread> threads;
  auto rasterization_lambda = [](Scene &scene, int start_row, int block_row) {
    for (auto &&obj : scene.objects) {
      obj->rasterization_block(scene, *obj, start_row, 0, block_row,
                               scene.width);
    }
  };
  for (int i = 0; i < thread_num - 1; ++i) {
    threads.emplace_back(rasterization_lambda, std::ref(*this),
                         thread_render_row_num * i, thread_render_row_num);
  }
  threads.emplace_back(rasterization_lambda, std::ref(*this),
                       thread_render_row_num * (thread_num - 1),
                       height - thread_render_row_num * (thread_num - 1));
  for (auto &&thread : threads) {
    thread.join();
  }
  threads.clear();
  int box_radius = roundf(4.0f * std::max(width, height) / 1024.0f);
  int penumbra_width = ceilf(width * 0.25f);
  int penumbra_height = ceilf(height * 0.25f);
  int penumbra_mask_thread_num = std::min(penumbra_height, maximum_thread_num);
  int penumbra_mask_thread_row_num =
      ceilf(penumbra_height * 1.0f / penumbra_mask_thread_num);
  auto penumbra_mask_lambda = [](Scene &scene, int start_row, int block_row) {
    for (auto &&light : scene.lights) {
      light->generate_penumbra_mask_block(scene, start_row, 0, block_row,
                                          ceilf(scene.width * 0.25f));
    }
  };
  for (int i = 0; i < penumbra_mask_thread_num - 1; ++i) {
    threads.emplace_back(penumbra_mask_lambda, std::ref(*this),
                         penumbra_mask_thread_row_num * i,
                         penumbra_mask_thread_row_num);
  }
  threads.emplace_back(penumbra_mask_lambda, std::ref(*this),
                       penumbra_mask_thread_row_num * (thread_num - 1),
                       penumbra_height - penumbra_mask_thread_row_num *
                                             (penumbra_mask_thread_num - 1));
  for (auto &&thread : threads) {
    thread.join();
  }
  threads.clear();
  for (auto &&light : lights) {
    light->box_blur_penumbra_mask(box_radius);
  }
  auto render_lambda = [](Scene &scene, int start_row, int block_row) {
    for (auto &&obj : scene.objects) {
      scene.shader(scene, start_row, 0, block_row, scene.width);
    }
  };
  for (int i = 0; i < thread_num - 1; ++i) {
    threads.emplace_back(render_lambda, std::ref(*this),
                         thread_render_row_num * i, thread_render_row_num);
  }
  threads.emplace_back(render_lambda, std::ref(*this),
                       thread_render_row_num * (thread_num - 1),
                       height - thread_render_row_num * (thread_num - 1));
  for (auto &&thread : threads) {
    thread.join();
  }
}

void Scene::add_model(const std::shared_ptr<Model> &model) {
  objects.push_back(model);
}

void Scene::add_light(const std::shared_ptr<light> &light) {
  lights.push_back(light);
}

void Scene::save_to_file(std::string filename) {
  std::vector<unsigned char> data(width * height * 3);
  for (int y = 0; y != height; ++y) {
    for (int x = 0; x != width; ++x) {
      data[3 * (y * width + x)] =
          std::clamp(frame_buffer[width * (height - y - 1) + x].x(), 0.0f,
                     1.0f) *
          255;
      data[3 * (y * width + x) + 1] =
          std::clamp(frame_buffer[width * (height - y - 1) + x].y(), 0.0f,
                     1.0f) *
          255;
      data[3 * (y * width + x) + 2] =
          std::clamp(frame_buffer[width * (height - y - 1) + x].z(), 0.0f,
                     1.0f) *
          255;
    }
  }
  stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}

int Scene::get_index(int x, int y) const { return width * y + x; }

void Scene::set_eye_pos(const Eigen::Vector3f &eye_pos) {
  this->eye_pos = eye_pos;
}

void Scene::set_view_dir(const Eigen::Vector3f &view_dir) {
  this->view_dir = view_dir.normalized();
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

float Scene::get_zNear() const { return zNear; }

float Scene::get_zFar() const { return zFar; }

int Scene::get_width() const { return width; }

int Scene::get_height() const { return height; }
std::function<void(Scene &Scene, int start_row, int start_col, int block_row,
                   int block_col)>
Scene::get_shader() const {
  return shader;
}