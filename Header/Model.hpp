#pragma once
#include "Eigen/Core"
#include <Triangle.hpp>
#include <global.hpp>
#include <vector>

struct Model : public Object {
  void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                     const Eigen::Matrix<float, 3, 3> &normal_mvp,
                     Scene *scene) override;
  ~Model();
  std::vector<Object *> objects;
  Texture *texture = nullptr;
  Texture *normal_texture = nullptr;
};