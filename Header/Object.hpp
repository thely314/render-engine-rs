#pragma once
#include "Eigen/Core"
#include "global.hpp"
#include <vector>
class Object {
public:
  virtual void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                             Scene &scene, const Model &model) = 0;
  virtual void rasterization_shadow_map(const Eigen::Matrix<float, 4, 4> &mvp,
                                        spot_light &light) = 0;
  virtual void clip(const Eigen::Matrix<float, 4, 4> &mvp,
                    std::vector<Object *> &objects) = 0;
  virtual void move(const Eigen::Matrix<float, 4, 4> &modeling_matrix) = 0;

  virtual ~Object() {};
};