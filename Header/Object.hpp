#pragma once
#include "Eigen/Core"
#include "global.hpp"
class Object {
public:
  virtual void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                             Scene &scene, const Model &model) = 0;
  virtual void rasterization_shadow_map(const Eigen::Matrix<float, 4, 4> &mvp,
                                        spot_light &light) = 0;
  virtual void rasterization_block(const Eigen::Matrix<float, 4, 4> &mvp,
                                   Scene &scene, const Model &model,
                                   int start_row, int start_col, int block_row,
                                   int block_col);
  virtual void
  rasterization_shadow_map_block(const Eigen::Matrix<float, 4, 4> &mvp,
                                 spot_light &light, int start_row,
                                 int start_col, int block_row, int block_col);
  virtual void clip(const Eigen::Matrix<float, 4, 4> &mvp, Model &parent) = 0;
  virtual void move(const Eigen::Matrix<float, 4, 4> &modeling_matrix) = 0;

  virtual ~Object() {};
};