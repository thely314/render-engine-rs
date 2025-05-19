#include "Model.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "light.hpp"
#include <memory>

void Model::set_pos(const Eigen::Vector3f &pos) {
  Eigen::Vector3f movement = pos - this->pos;
  this->pos = pos;
  Eigen::Matrix<float, 4, 4> modeling_matrix;
  modeling_matrix << 1.0f, 0.0f, 0.0f, movement.x(), 0.0f, 1.0f, 0.0f,
      movement.y(), 0.0f, 0.0f, 1.0f, movement.z(), 0.0f, 0.0f, 0.0f, 1.0f;
  for (auto &&obj : sub_models) {
    obj->modeling(modeling_matrix);
  }
  for (auto &&obj : triangles) {
    obj.modeling(modeling_matrix);
  }
}

Eigen::Vector3f Model::get_pos() const { return pos; }

void Model::set_scale(float rate) {
  float scale_rate = rate / scale;
  scale = rate;
  Eigen::Matrix<float, 4, 4> scale_matrix;
  scale_matrix << scale_rate, 0.0f, 0.0f, 0.0f, 0.0f, scale_rate, 0.0f, 0.0f,
      0.0f, 0.0f, scale_rate, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f;
  for (auto &&obj : sub_models) {
    obj->modeling(scale_matrix);
  }
  for (auto &&obj : triangles) {
    obj.modeling(scale_matrix);
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

void Model::modeling(const Eigen::Matrix<float, 4, 4> &matrix) {
  for (auto &&obj : sub_models) {
    obj->modeling(matrix);
  }
  for (auto &&obj : triangles) {
    obj.modeling(matrix);
  }
}

void Model::add(const std::shared_ptr<Model> &obj) {
  sub_models.push_back(obj);
}
void Model::add(const Triangle &obj) { triangles.push_back(obj); }

Model::~Model() {}

void Model::rasterization(const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene,
                          const Model &model) {
  int i = 0;
  for (auto &&obj : sub_models) {
    obj->rasterization(mvp, scene, *this);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization(mvp, scene, *this);
  }
}
void Model::rasterization_shadow_map(const Eigen::Matrix<float, 4, 4> &mvp,
                                     spot_light &light) {
  for (auto &&obj : sub_models) {
    obj->rasterization_shadow_map(mvp, light);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_shadow_map(mvp, light);
  }
}
void Model::rasterization_block(const Eigen::Matrix<float, 4, 4> &mvp,
                                Scene &scene, const Model &model, int start_row,
                                int start_col, int block_row, int block_col) {
  for (auto &&obj : sub_models) {
    obj->rasterization_block(mvp, scene, *this, start_row, start_col, block_row,
                             block_col);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_block(mvp, scene, *this, start_row, start_col, block_row,
                            block_col);
  }
}
void Model::rasterization_shadow_map_block(
    const Eigen::Matrix<float, 4, 4> &mvp, spot_light &light, int start_row,
    int start_col, int block_row, int block_col) {
  for (auto &&obj : sub_models) {
    obj->rasterization_shadow_map_block(mvp, light, start_row, start_col,
                                        block_row, block_col);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_shadow_map_block(mvp, light, start_row, start_col,
                                       block_row, block_col);
  }
}

void Model::rasterization_shadow_map_block(
    const Eigen::Matrix<float, 4, 4> &mvp, directional_light &light,
    int start_row, int start_col, int block_row, int block_col) {
  for (auto &&obj : sub_models) {
    obj->rasterization_shadow_map_block(mvp, light, start_row, start_col,
                                        block_row, block_col);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_shadow_map_block(mvp, light, start_row, start_col,
                                       block_row, block_col);
  }
}

void Model::to_NDC(int width, int height) {
  for (auto &&obj : sub_models) {
    obj->to_NDC(width, height);
  }
  for (auto &&obj : clip_triangles) {
    obj.to_NDC(width, height);
  }
}

void Model::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                 const Eigen::Matrix<float, 4, 4> &mv, Model &parent) {
  // TODO:上锁
  clip_triangles.clear();
  for (auto &&obj : sub_models) {
    obj->clip(mvp, mv, *this);
  }
  for (auto &&obj : triangles) {
    obj.clip(mvp, mv, *this);
  }
}
void Model::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                 const Eigen::Matrix<float, 4, 4> &mv) {
  clip_triangles.clear();
  for (auto &&obj : sub_models) {
    obj->clip(mvp, mv, *this);
  }
  for (auto &&obj : triangles) {
    obj.clip(mvp, mv, *this);
  }
}
