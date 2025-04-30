#pragma once
#include "global.hpp"

struct Object {
  virtual void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                             const Eigen::Matrix<float, 3, 3> &normal_mvp,
                             Scene &scene) = 0;
  virtual ~Object() {};
};