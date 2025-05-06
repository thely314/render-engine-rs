#pragma once
#include "Eigen/Core"
#include "global.hpp"

struct Object {
  virtual void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                             const Eigen::Matrix<float, 3, 3> &normal_mvp,
                             Scene &scene, const Model &model) = 0;
  virtual void generate_shadowmap(const Eigen::Matrix<float, 4, 4> &mvp,
                                  const Eigen::Matrix<float, 3, 3> &normal_mvp,
                                  spot_light &light) = 0;
  virtual void move(const Eigen::Matrix<float, 4, 4> &modeling_matrix) = 0;
  virtual ~Object() {};
};