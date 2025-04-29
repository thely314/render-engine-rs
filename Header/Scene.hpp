#pragma once
#include "Eigen/Core"
#include "Object.hpp"
#include "global.hpp"
#include <string>
#include <vector>

struct light {
  Eigen::Vector3f intensity;
  Eigen::Vector3f pos;
};
struct Scene {
  Scene(int width, int height);
  void start_render();
  void add_model(Object *);
  void save_to_file(std::string filename);
  ~Scene();
  int get_index(int x, int y);
  void set_eye_pos(const Eigen::Vector3f &eye_pos);
  void set_view_dir(const Eigen::Vector3f &view_dir);
  void set_zNear(float zNear);
  void set_zFar(float zFar);
  std::vector<Object *> objects;
  std::vector<light> lights;
  std::vector<Eigen::Vector3f> frame_buffer;
  std::vector<float> z_buffer;
  Eigen::Vector3f eye_pos;
  Eigen::Vector3f view_dir;
  float zNear;
  float zFar;
  int width;
  int height;
};