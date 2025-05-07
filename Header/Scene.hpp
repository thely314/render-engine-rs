#pragma once
#include "Eigen/Core"
#include "Model.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "light.hpp"
#include <functional>
#include <string>
#include <vector>

struct Scene {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct light;
  friend struct spot_light;
  Scene(int width, int height);
  void start_render();
  void add_model(Model *);
  void save_to_file(std::string filename);
  ~Scene();
  int get_index(int x, int y);
  void set_eye_pos(const Eigen::Vector3f &eye_pos);
  void set_view_dir(const Eigen::Vector3f &view_dir);
  void set_zNear(float zNear);
  void set_zFar(float zFar);
  std::vector<Model *> objects;
  std::vector<light *> lights;
  std::vector<Eigen::Vector3f> frame_buffer;
  std::vector<float> z_buffer;
  Eigen::Vector3f eye_pos;
  Eigen::Vector3f view_dir;
  float zNear;
  float zFar;
  int width;
  int height;
  std::function<Eigen::Vector3f(Vertex_rasterization &, const Scene &,
                                const Model &, const Eigen::Vector3f &,
                                const Eigen::Vector3f &)>
      shader;
};