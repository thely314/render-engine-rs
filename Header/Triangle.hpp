#pragma once
#include "Eigen/Core"
#include "Object.hpp"
#include "Scene.hpp"
#include <Texture.hpp>
#include <tuple>
#include <vector>
struct Vertex_vec4 {
  // 这三个运算符是给插值运算用的
  friend Vertex_vec4 operator+(const Vertex_vec4 &x, const Vertex_vec4 &y) {
    return Vertex_vec4{x.pos + y.pos, (x.normal + y.normal).normalized(),
                       x.color + y.color, x.texture_coords + y.texture_coords};
  }
  friend Vertex_vec4 operator*(float rate, const Vertex_vec4 &v) {
    return Vertex_vec4{rate * v.pos, rate * v.normal, rate * v.color,
                       rate * v.texture_coords};
  }
  friend Vertex_vec4 operator*(const Vertex_vec4 &v, float rate) {
    return Vertex_vec4{rate * v.pos, rate * v.normal, rate * v.color,
                       rate * v.texture_coords};
  }
  Eigen::Vector4f pos;
  Eigen::Vector3f normal;
  Eigen::Vector3f color;
  Eigen::Vector2f texture_coords;
};
struct Triangle_Vec4 {
  Triangle_Vec4() = default;
  Triangle_Vec4(const Vertex_vec4 &v0, const Vertex_vec4 &v1,
                const Vertex_vec4 &v2)
      : vertexs{v0, v1, v2} {}
  friend void crop_triangles_zNear(std::vector<Triangle_Vec4> &triangles,
                                   float zNear);
  friend void crop_triangles_zFar(std::vector<Triangle_Vec4> &triangles,
                                  float zFar);
  friend void crop_triangles_xLeft(std::vector<Triangle_Vec4> &triangles);
  friend void crop_triangles_xRight(std::vector<Triangle_Vec4> &triangles);
  friend void crop_triangles_yBottom(std::vector<Triangle_Vec4> &triangles);
  friend void crop_triangles_yTop(std::vector<Triangle_Vec4> &triangles);
  void rasterization(Scene *scene);
  Vertex_vec4 vertexs[3];
};

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
struct Triangle : public Object {
  Triangle() = default;
  Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2);
  void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                     const Eigen::Matrix<float, 3, 3> &normal_mvp,
                     Scene *scene) override;
  static std::tuple<float, float, float> cal_bary_coord_2D(Eigen::Vector2f v0,
                                                           Eigen::Vector2f v1,
                                                           Eigen::Vector2f v2,
                                                           Eigen::Vector2f p);
  static bool inside_triangle(float alpha, float beta, float gamma);

  Vertex vertexs[3];
};
