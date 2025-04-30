#include "Model.hpp"
#include "Eigen/Core"
#include "Texture.hpp"
#include <iostream>
#include <memory>
Model::~Model() {
  for (auto obj : objects) {
    delete obj;
  }
}
void Model::rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                          const Eigen::Matrix<float, 3, 3> &normal_mvp,
                          Scene &scene, const Model &model) {
  int i = 0;
  // std::cout << mvp;
  for (auto obj : objects) {
    obj->rasterization(mvp, normal_mvp, scene, *this);
    // std::cout << ++i << "\n";
  }
}
void Model::set_pos(const Eigen::Vector3f &pos) {
  Eigen::Vector3f movement = pos - this->pos;
  this->pos = pos;
  Eigen::Matrix<float, 4, 4> modeling_matrix;
  modeling_matrix << 1.0f, 0.0f, 0.0f, movement.x(), 0.0f, 1.0f, 0.0f,
      movement.y(), 0.0f, 0.0f, 1.0f, movement.z(), 0.0f, 0.0f, 0.0f, 1.0f;
  for (auto obj : objects) {
    obj->move(modeling_matrix);
  }
}
void Model::move(const Eigen::Matrix<float, 4, 4> &matrix) {
  for (auto obj : objects) {
    obj->move(matrix);
  }
}
void Model::set_scale(float rate) {
  float scale_rate = rate / scale;
  scale = rate;
  Eigen::Matrix<float, 4, 4> scale_matrix;
  scale_matrix << scale_rate, 0.0f, 0.0f, 0.0f, 0.0f, scale_rate, 0.0f, 0.0f,
      0.0f, 0.0f, scale_rate, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f;
  for (auto obj : objects) {
    obj->move(scale_matrix);
  }
}
void Model::set_texture(const std::shared_ptr<Texture> &texture) {
  textures[TEXTURE] = texture;
}
void Model::set_normal_texture(const std::shared_ptr<Texture> &texture) {
  textures[NORMAL_TEXTURE] = texture;
}