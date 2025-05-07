#include "Model.hpp"
#include "Texture.hpp"
#include <memory>

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

Eigen::Vector3f Model::get_pos() const { return pos; }

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

float Model::get_scale() const { return scale; }

void Model::set_texture(const std::shared_ptr<Texture> &texture,
                        Model::TEXTURES id) {
  textures[id] = texture;
}

std::shared_ptr<Texture> Model::get_texture(Model::TEXTURES id) const {
  return textures[id];
}

void Model::move(const Eigen::Matrix<float, 4, 4> &matrix) {
  for (auto obj : objects) {
    obj->move(matrix);
  }
}

void Model::add(Object *obj) { objects.push_back(obj); }

Model::~Model() {
  for (auto obj : objects) {
    delete obj;
  }
}

void Model::rasterization(const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene,
                          const Model &model) {
  for (auto obj : clip_objects) {
    obj->rasterization(mvp, scene, *this);
  }
  clip_objects.clear();
}
void Model::rasterization_shadow_map(const Eigen::Matrix<float, 4, 4> &mvp,
                                     spot_light &light) {
  for (auto obj : clip_objects) {
    obj->rasterization_shadow_map(mvp, light);
  }
  clip_objects.clear();
}
void Model::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                 std::vector<Object *> &objects) {
  // TODO:上锁
  objects.push_back(this);
  for (auto obj : this->objects) {
    obj->clip(mvp, this->clip_objects);
  }
}
void Model::clip(const Eigen::Matrix<float, 4, 4> &mvp) {
  for (auto obj : this->objects) {
    obj->clip(mvp, this->clip_objects);
  }
}
