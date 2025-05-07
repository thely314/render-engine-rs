#pragma once
#include "Eigen/Core"
#include <Eigen/Dense>
#include <vector>
struct Vertex;
struct Vertex_rasterization;
struct Scene;
class light {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct spot_light;

public:
  light();
  light(const Eigen::Vector3f &pos, const Eigen::Vector3f &intensity);
  Eigen::Vector3f get_pos() const;
  void set_pos(const Eigen::Vector3f &pos);
  Eigen::Vector3f get_intensity() const;
  void set_intensity(const Eigen::Vector3f &intensity);
  virtual void look_at(const Scene &);
  virtual bool in_shadow(Vertex_rasterization &);
  virtual ~light() = default;

protected:
  Eigen::Vector3f pos;
  Eigen::Vector3f intensity;
};
class spot_light : public light {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct Scene;

public:
  spot_light();
  Eigen::Vector3f get_light_dir() const;
  void set_light_dir(const Eigen::Vector3f &dir);
  float get_fov() const;
  void set_fov(float fov);
  float get_aspect_ratio() const;
  void set_aspect_ratio(float aspect_ratio);
  float get_zNear() const;
  void set_zNear(float zNear);
  float get_zFar() const;
  void set_zFar(float zFar);
  int get_width() const;
  void set_width(int width);
  int get_height() const;
  void set_height(int height);

private:
  Eigen::Vector3f light_dir;
  float fov;
  float aspect_ratio;
  float zNear;
  float zFar;
  int zbuffer_width;
  int zbuffer_height;
  Eigen::Matrix<float, 4, 4> mvp;
  std::vector<float> z_buffer;
  int get_index(int x, int y);
  void look_at(const Scene &) override;
  bool in_shadow(Vertex_rasterization &) override;
};