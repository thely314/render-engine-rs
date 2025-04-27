#pragma once
#include "Eigen/Core"
#include <Eigen/Dense>
#include <Texture.hpp>

struct Vertex {
  Eigen::Vector3f pos;
  Eigen::Vector3f normal;
  Eigen::Vector3f color;
  Eigen::Vector2f texture_coords;
  Eigen::Vector2f normal_texture_coords;
};
struct Triangle {
  Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2,
           Texture *tex_ptr = nullptr, Texture *normal_tex_ptr = nullptr)
      : v0(v0), v1(v1), v2(v2), texture(tex_ptr),
        normal_texture(normal_tex_ptr) {}
  Vertex v0, v1, v2;
  Texture *texture;
  Texture *normal_texture;
};