#pragma once
#include "Eigen/Core"
#include "Model.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "light.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
constexpr float gaussian_blur_horizontal[9] = {
    0.01621622, 0.05405405, 0.12162162, 0.19459459, 0.22702703,
    0.19459459, 0.12162162, 0.05405405, 0.01621622};
constexpr float gaussian_blur_vertical[5] = {0.07027027, 0.31621622, 0.22702703,
                                             0.31621622, 0.07027027};
constexpr float gaussian_blur_horizontal_offset[9] = {-4, -3, -2, -1, 0,
                                                      1,  2,  3,  4};
constexpr float gaussian_blur_vertical_offset[5] = {-3.23076923, -1.38461538, 0,
                                                    1.38461538, 3.23076923};

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

  int get_index(int x, int y) const;
  int get_penumbra_mask_index(int x, int y) const;
  void set_eye_pos(const Eigen::Vector3f &eye_pos);
  void set_view_dir(const Eigen::Vector3f &view_dir);
  void set_zNear(float zNear);
  void set_zFar(float zFar);
  void set_width(int width);
  void set_height(int height);
  void set_penumbra_mask_status(bool status);
  void set_shader(
      const std::function<void(Scene &Scene, int start_row, int start_col,
                               int block_row, int block_col)> &shader);
  Eigen::Vector3f get_eye_pos() const;
  Eigen::Vector3f get_view_dir() const;
  float get_zNear() const;
  float get_zFar() const;
  int get_width() const;
  int get_height() const;
  bool get_penumbra_mask_status() const;
  std::function<void(Scene &Scene, int start_row, int start_col, int block_row,
                     int block_col)>
  get_shader() const;
  std::vector<Eigen::Vector3f> frame_buffer;
  std::vector<Eigen::Vector3f> pos_buffer;
  std::vector<Eigen::Vector3f> normal_buffer;
  std::vector<Eigen::Vector3f> diffuse_buffer;
  std::vector<Eigen::Vector3f> specular_buffer;
  std::vector<Eigen::Vector3f> glow_buffer;
  std::vector<float> z_buffer;
  std::vector<std::shared_ptr<Model>> objects;
  std::vector<std::shared_ptr<light>> lights;
  std::vector<std::vector<light::SHADOW_STATUS>> penumbra_masks;
  std::vector<std::vector<float>> penumbra_masks_blur;

private:
  std::vector<float>
  penumbra_mask_blur_horizontal(const std::vector<float> &input);
  std::vector<float>
  penumbra_mask_blur_vertical(const std::vector<float> &input);
  Eigen::Vector3f eye_pos;
  Eigen::Vector3f view_dir;
  float zNear;
  float zFar;
  int width;
  int height;
  int penumbra_mask_width;
  int penumbra_mask_height;
  bool enable_penumbra_mask;
  std::function<void(Scene &Scene, int start_row, int start_col, int block_row,
                     int block_col)>
      shader;
};