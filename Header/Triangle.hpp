#pragma once
#include "Eigen/Core"
#include "Object.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
#include <tuple>
#include <vector>

struct Model;
struct Vertex {
  // 这三个运算符是给插值运算用的
  friend Vertex operator+(const Vertex &x, const Vertex &y) {
    return Vertex{x.pos + y.pos,
                  (x.normal + y.normal).normalized(),
                  x.color + y.color,
                  x.texture_coords + y.texture_coords,
                  x.transform_pos + y.transform_pos,
                  (x.tranfrom_normal + y.tranfrom_normal).normalized()};
  }
  friend Vertex operator*(float rate, const Vertex &v) {
    return Vertex{rate * v.pos,           rate * v.normal,
                  rate * v.color,         rate * v.texture_coords,
                  rate * v.transform_pos, rate * v.tranfrom_normal};
  }
  friend Vertex operator*(const Vertex &v, float rate) {
    return Vertex{rate * v.pos,           rate * v.normal,
                  rate * v.color,         rate * v.texture_coords,
                  rate * v.transform_pos, rate * v.tranfrom_normal};
  }
  Eigen::Vector3f pos;
  Eigen::Vector3f normal;
  Eigen::Vector3f color;
  Eigen::Vector2f texture_coords;
  Eigen::Vector4f transform_pos;
  Eigen::Vector3f tranfrom_normal;
};
struct Triangle : public Object {
  Triangle() = default;
  Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2);
  void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                     const Eigen::Matrix<float, 3, 3> &normal_mvp, Scene &scene,
                     const Model &model) override;
  void move(const Eigen::Matrix<float, 4, 4> &modeling_martix) override;
  static std::tuple<float, float, float> cal_bary_coord_2D(Eigen::Vector2f v0,
                                                           Eigen::Vector2f v1,
                                                           Eigen::Vector2f v2,
                                                           Eigen::Vector2f p);
  static bool inside_triangle(float alpha, float beta, float gamma);
  friend void crop_triangles_zNear(std::vector<Triangle> &triangles,
                                   float zNear);
  friend void crop_triangles_zFar(std::vector<Triangle> &triangles, float zFar);
  friend void crop_triangles_xLeft(std::vector<Triangle> &triangles);
  friend void crop_triangles_xRight(std::vector<Triangle> &triangles);
  friend void crop_triangles_yBottom(std::vector<Triangle> &triangles);
  friend void crop_triangles_yTop(std::vector<Triangle> &triangles);
  void draw(Scene &scene, const Model &model);
  Vertex vertexs[3];
};
