#pragma once
#include "Eigen/Core"
#include <functional>
#include <tuple>
#include <vector>

class Model;
class Scene;
class TriangleRasterization;
class Vertex {
  // 这三个运算符是给插值运算用的
public:
  friend Vertex operator+(const Vertex &x, const Vertex &y) {
    return Vertex{x.pos + y.pos, (x.normal + y.normal).normalized(),
                  x.color + y.color, x.texture_coords + y.texture_coords};
  }
  friend Vertex operator*(float rate, const Vertex &v) {
    return Vertex{rate * v.pos, rate * v.normal, rate * v.color,
                  rate * v.texture_coords};
  }
  friend Vertex operator*(const Vertex &v, float rate) {
    return Vertex{rate * v.pos, rate * v.normal, rate * v.color,
                  rate * v.texture_coords};
  }
  Eigen::Vector3f pos;
  Eigen::Vector3f normal;
  Eigen::Vector3f color;
  Eigen::Vector2f texture_coords;
};
class Triangle {

  friend class Scene;
  friend class Model;
  friend class light;
  friend class SpotLight;

public:
  Triangle() = default;
  Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2);
  void modeling(const Eigen::Matrix<float, 4, 4> &modeling_martix);
  Vertex vertexs[3];

private:
  void rasterization_block(Scene &scene, const Model &model, int start_row,
                           int start_col, int block_row, int block_col) {}
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv, Model &parent);
};

class VertexRasterization {
public:
  // 这三个运算符是给插值运算用的
  friend VertexRasterization operator+(const VertexRasterization &x,
                                       const VertexRasterization &y) {
    return VertexRasterization{
        x.pos + y.pos, (x.normal + y.normal).normalized(), x.color + y.color,
        x.texture_coords + y.texture_coords, x.transform_pos + y.transform_pos};
  }
  friend VertexRasterization operator*(float rate,
                                       const VertexRasterization &v) {
    return VertexRasterization{rate * v.pos, rate * v.normal, rate * v.color,
                               rate * v.texture_coords, rate * v.transform_pos};
  }
  friend VertexRasterization operator*(const VertexRasterization &v,
                                       float rate) {
    return VertexRasterization{rate * v.pos, rate * v.normal, rate * v.color,
                               rate * v.texture_coords, rate * v.transform_pos};
  }
  Eigen::Vector3f pos;
  Eigen::Vector3f normal;
  Eigen::Vector3f color;
  Eigen::Vector2f texture_coords;
  Eigen::Vector4f transform_pos;
};
class TriangleRasterization {
  friend class Triangle;
  friend class Scene;
  friend class Model;
  friend class light;
  friend class SpotLight;

public:
  TriangleRasterization() = default;
  TriangleRasterization(const Triangle &normal_triangle);
  TriangleRasterization(const VertexRasterization &v0,
                        const VertexRasterization &v1,
                        const VertexRasterization &v2);
  void modeling(const Eigen::Matrix<float, 4, 4> &modeling_martix);
  VertexRasterization vertexs[3];

private:
  void rasterization_block(Scene &scene, const Model &model, int start_row,
                           int start_col, int block_row, int block_col);
  template <bool IsProjection>
  void rasterization_shadow_map_block(
      std::vector<float> &z_buffer, int start_row, int start_col, int block_row,
      int block_col,
      const std::function<float(float, float)> &depth_transformer,
      const std::function<int(int, int)> &get_index) {
    if (block_row <= 0 || block_col <= 0) {
      return;
    }
    int box_left = (int)std::min(
        vertexs[0].transform_pos.x(),
        std::min(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
    int box_right = std::max(
        vertexs[0].transform_pos.x(),
        std::max(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
    int box_bottom = std::min(
        vertexs[0].transform_pos.y(),
        std::min(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
    int box_top = std::max(
        vertexs[0].transform_pos.y(),
        std::max(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
    box_top = std::clamp(box_top, start_row, start_row + block_row - 1);
    box_bottom = std::clamp(box_bottom, start_row, start_row + block_row - 1);
    box_left = std::clamp(box_left, start_col, start_col + block_col - 1);
    box_right = std::clamp(box_right, start_col, start_col + block_col - 1);
    for (int y = box_bottom; y <= box_top; ++y) {
      for (int x = box_left; x <= box_right; ++x) {
        auto [alpha, beta, gamma] =
            TriangleRasterization::compute_bary_coord_2D(
                vertexs[0].transform_pos.head(2),
                vertexs[1].transform_pos.head(2),
                vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
        if (TriangleRasterization::inside_triangle(alpha, beta, gamma)) {
          int idx = get_index(x, y);
          if constexpr (IsProjection) {
            alpha = alpha / vertexs[0].transform_pos.w();
            beta = beta / vertexs[1].transform_pos.w();
            gamma = gamma / vertexs[2].transform_pos.w();
            float w_inter = 1.0f / (alpha + beta + gamma);
            alpha *= w_inter;
            beta *= w_inter;
            gamma *= w_inter;
          }
          float point_transform_pos_z = alpha * vertexs[0].transform_pos.z() +
                                        beta * vertexs[1].transform_pos.z() +
                                        gamma * vertexs[2].transform_pos.z();
          float point_transform_pos_w = alpha * vertexs[0].transform_pos.w() +
                                        beta * vertexs[1].transform_pos.w() +
                                        gamma * vertexs[2].transform_pos.w();
          float depth =
              depth_transformer(point_transform_pos_z, point_transform_pos_w);
          if (depth > z_buffer[idx]) {
            z_buffer[idx] = depth;
          }
        }
      }
    }
  }
  void to_NDC(int width, int height);
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv, Model &parent) {}
  static std::tuple<float, float, float>
  compute_bary_coord_2D(Eigen::Vector2f v0, Eigen::Vector2f v1,
                        Eigen::Vector2f v2, Eigen::Vector2f p);
  static bool inside_triangle(float alpha, float beta, float gamma);
};