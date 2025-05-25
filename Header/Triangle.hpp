#pragma once
#include "Eigen/Core"
#include "Object.hpp"
#include "Scene.hpp"
#include "light.hpp"
#include <tuple>
#include <vector>

struct Model;
struct Vertex {
  // 这三个运算符是给插值运算用的
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
class Triangle : public Object {

  friend struct Scene;
  friend struct Model;
  friend struct light;
  friend struct spot_light;

public:
  Triangle() = default;
  Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2);
  void modeling(const Eigen::Matrix<float, 4, 4> &modeling_martix) override;
  Vertex vertexs[3];

private:
  void rasterization_block(Scene &scene, const Model &model, int start_row,
                           int start_col, int block_row,
                           int block_col) override {}
  void rasterization_shadow_map_block(spot_light &light, int start_row,
                                      int start_col, int block_row,
                                      int block_col) override {}
  void rasterization_shadow_map_block(directional_light &light, int start_row,
                                      int start_col, int block_row,
                                      int block_col) override {}
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv, Model &parent) override;
};

struct Vertex_rasterization {
  // 这三个运算符是给插值运算用的
  friend Vertex_rasterization operator+(const Vertex_rasterization &x,
                                        const Vertex_rasterization &y) {
    return Vertex_rasterization{
        x.pos + y.pos, (x.normal + y.normal).normalized(), x.color + y.color,
        x.texture_coords + y.texture_coords, x.transform_pos + y.transform_pos};
  }
  friend Vertex_rasterization operator*(float rate,
                                        const Vertex_rasterization &v) {
    return Vertex_rasterization{rate * v.pos, rate * v.normal, rate * v.color,
                                rate * v.texture_coords,
                                rate * v.transform_pos};
  }
  friend Vertex_rasterization operator*(const Vertex_rasterization &v,
                                        float rate) {
    return Vertex_rasterization{rate * v.pos, rate * v.normal, rate * v.color,
                                rate * v.texture_coords,
                                rate * v.transform_pos};
  }
  Eigen::Vector3f pos;
  Eigen::Vector3f normal;
  Eigen::Vector3f color;
  Eigen::Vector2f texture_coords;
  Eigen::Vector4f transform_pos;
};
class Triangle_rasterization : public Object {
  friend struct Triangle;
  friend struct Scene;
  friend struct Model;
  friend struct light;
  friend struct spot_light;

public:
  Triangle_rasterization() = default;
  Triangle_rasterization(const Triangle &normal_triangle);
  Triangle_rasterization(const Vertex_rasterization &v0,
                         const Vertex_rasterization &v1,
                         const Vertex_rasterization &v2);
  void modeling(const Eigen::Matrix<float, 4, 4> &modeling_martix) override;
  Vertex_rasterization vertexs[3];

private:
  void rasterization_block(Scene &scene, const Model &model, int start_row,
                           int start_col, int block_row,
                           int block_col) override;
  void rasterization_shadow_map_block(spot_light &light, int start_row,
                                      int start_col, int block_row,
                                      int block_col) override;
  void rasterization_shadow_map_block(directional_light &light, int start_row,
                                      int start_col, int block_row,
                                      int block_col) override;
  void to_NDC(int width, int height);
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv, Model &parent) override {}
  template <int N, bool isLess>
  friend void clip_triangles(std::vector<Triangle_rasterization> &triangles);
  static std::tuple<float, float, float> cal_bary_coord_2D(Eigen::Vector2f v0,
                                                           Eigen::Vector2f v1,
                                                           Eigen::Vector2f v2,
                                                           Eigen::Vector2f p);
  static bool inside_triangle(float alpha, float beta, float gamma);
};