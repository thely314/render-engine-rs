#pragma once
#include "Eigen/Core"
#include "global.hpp"
class Object {
public:
  virtual void rasterization_block(Scene &scene, const Model &model,
                                   int start_row, int start_col, int block_row,
                                   int block_col);
  virtual void rasterization_shadow_map_block(spot_light &light, int start_row,
                                              int start_col, int block_row,
                                              int block_col);
  virtual void rasterization_shadow_map_block(directional_light &light,
                                              int start_row, int start_col,
                                              int block_row, int block_col);
  virtual void clip(const Eigen::Matrix<float, 4, 4> &mvp,
                    const Eigen::Matrix<float, 4, 4> &mv, Model &parent) = 0;
  virtual void modeling(const Eigen::Matrix<float, 4, 4> &modeling_matrix) = 0;

  virtual ~Object() {};
};