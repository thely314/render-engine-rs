#pragma once
#include "Eigen/Core"
#include "Model.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "light.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Scene {
public:
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct light;
  friend struct spot_light;
  Scene(int width, int height);
  ~Scene() = default;
  void start_render();
  void add_model(const std::shared_ptr<Model> &model);
  void add_light(const std::shared_ptr<light> &light);
  void save_to_file(std::string filename);

  int get_index(int x, int y);
  void set_eye_pos(const Eigen::Vector3f &eye_pos);
  void set_view_dir(const Eigen::Vector3f &view_dir);
  void set_zNear(float zNear);
  void set_zFar(float zFar);
  void set_width(int width);
  void set_height(int height);
  void
  set_shader(const std::function<Eigen::Vector3f(
                 Vertex_rasterization &, const Scene &, const Model &,
                 const Eigen::Vector3f &, const Eigen::Vector3f &)> &shader);
  Eigen::Vector3f get_eye_pos() const;
  Eigen::Vector3f get_view_dir() const;
  float get_zNear() const;
  float get_zFar() const;
  int get_width() const;
  int get_height() const;
  std::function<Eigen::Vector3f(Vertex_rasterization &, const Scene &,
                                const Model &, const Eigen::Vector3f &,
                                const Eigen::Vector3f &)>
  get_shader() const;
  std::vector<std::shared_ptr<Model>> objects;
  std::vector<std::shared_ptr<light>> lights;

private:
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