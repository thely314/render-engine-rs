#pragma once
#include "Eigen/Core"
#include <Eigen/Dense>
#include <vector>
struct Vertex;
struct Scene;
struct light {
  light();
  light(const Eigen::Vector3f &pos, const Eigen::Vector3f &intensity);
  Eigen::Vector3f pos;
  Eigen::Vector3f intensity;
  virtual void look_at(const Scene &);
  virtual bool in_shadow(Vertex &);
  virtual ~light() = default;
};
struct spot_light : public light {
  Eigen::Vector3f light_dir;
  float fov;
  float aspect_ratio;
  float zNear;
  float zFar;
  int zbuffer_width;
  int zbuffer_height;
  Eigen::Matrix<float, 4, 4> mvp;
  std::vector<float> z_buffer;
  spot_light();
  int get_index(int x, int y);
  void look_at(const Scene &) override;
  bool in_shadow(Vertex &) override;
};