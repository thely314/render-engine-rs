#pragma once
#include "Eigen/Core"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <print>
#include <stb_image.h>
#include <vector>

struct Texture {
  Texture(const char *filename) {
    data = stbi_load(filename, &width, &height, &channels, 3);
    if (data == nullptr) {
      std::print("Error: could not open file {}\n", filename);
    }
  }
  int get_index(int x, int y) {
    return channels * (width * (height - y - 1) + x);
  };
  Eigen::Vector3f get_color(float u, float v) {
    assert(data != nullptr);
    u = std::clamp(0.0f, 1.0f - 0.0f, u);
    v = std::clamp(0.0f, 1.0f - 0.0f, v);
    Eigen::Vector2f matrix_pos{u * width, v * height};
    int center_x = round(matrix_pos.x()), center_y = round(matrix_pos.y());
    float h_rate = matrix_pos.x() + 0.5f - center_x,
          v_rate = matrix_pos.y() + 0.5f - center_y;
    if (center_x == 0 || center_x == width) {
      if (center_y == 0 || center_y == height) {
        int idx = get_index(matrix_pos.x(), matrix_pos.y());
        return Eigen::Vector3f(data[idx], data[idx + 1], data[idx + 2]) /
               255.0f;
      } else {
        int idx_top, idx_bottom;
        if (center_x == 0) {
          idx_top = get_index(center_x, center_y),
          idx_bottom = get_index(center_x, center_y - 1);
        } else {
          idx_top = get_index(center_x - 1, center_y),
          idx_bottom = get_index(center_x - 1, center_y - 1);
        }
        return Eigen::Vector3f(v_rate * data[idx_top] +
                                   (1 - v_rate) * data[idx_bottom],
                               v_rate * data[idx_top + 1] +
                                   (1 - v_rate) * data[idx_bottom + 1],
                               v_rate * data[idx_top + 2] +
                                   (1 - v_rate) * data[idx_bottom + 1]) /
               255.0f;
      }
    } else if (center_y == 0 || center_y == height) {
      int idx_left, idx_right;
      if (center_y == 0) {
        idx_left = get_index(center_x - 1, center_y),
        idx_right = get_index(center_x, center_y);
      } else {
        idx_left = get_index(center_x - 1, center_y - 1),
        idx_right = get_index(center_x, center_y - 1);
      }
      return Eigen::Vector3f(h_rate * data[idx_right] +
                                 (1 - h_rate) * data[idx_left],
                             h_rate * data[idx_right + 1] +
                                 (1 - h_rate) * data[idx_left + 1],
                             h_rate * data[idx_right + 2] +
                                 (1 - h_rate) * data[idx_left + 1]) /
             255.0f;
    } else {
      int idx[4]{get_index(center_x - 1, center_y),
                 get_index(center_x, center_y),
                 get_index(center_x - 1, center_y - 1),
                 get_index(center_x, center_y - 1)};
      Eigen::Vector3f color_top{
          (1 - h_rate) * data[idx[0]] + h_rate * data[idx[1]],
          (1 - h_rate) * data[idx[0] + 1] + h_rate * data[idx[1] + 1],
          (1 - h_rate) * data[idx[0] + 2] + h_rate * data[idx[1] + 2]};
      Eigen::Vector3f color_bottom{
          (1 - h_rate) * data[idx[2]] + h_rate * data[idx[3]],
          (1 - h_rate) * data[idx[2] + 1] + h_rate * data[idx[3] + 1],
          (1 - h_rate) * data[idx[2] + 2] + h_rate * data[idx[3] + 2]};
      return (v_rate * color_top + (1 - v_rate) * color_bottom) / 255.0f;
    }
  }
  ~Texture() { delete[] data; }
  unsigned char *data;
  int width;
  int height;
  int channels;
};