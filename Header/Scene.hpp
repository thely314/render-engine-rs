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
constexpr float gaussian_blur_horizontal[9] = {
    0.01621622, 0.05405405, 0.12162162, 0.19459459, 0.22702703,
    0.19459459, 0.12162162, 0.05405405, 0.01621622};
constexpr float gaussian_blur_vertical[5] = {0.07027027, 0.31621622, 0.22702703,
                                             0.31621622, 0.07027027};
constexpr float gaussian_blur_horizontal_offset[9] = {-4, -3, -2, -1, 0,
                                                      1,  2,  3,  4};
constexpr float gaussian_blur_vertical_offset[5] = {-3.23076923, -1.38461538, 0,
                                                    1.38461538, 3.23076923};
// 高斯模糊的半径最好要结合实际的分辨率和摄像机到物体的距离来计算
// 但是我懒，所以不写,直接把半径写死在9x9
// 而且高斯模糊的开销很大，不上mipmap效率可能还不如把mask关了，要高效率还要手写mipmap，太累
// 毕竟我连纹理那块都没上mipmap只上了双线性插值
// 如果真的想改我会去调OpenCV的,就算用HLSL写起码还有封好的mipmap
class Scene {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct light;
  friend struct spot_light;
  friend struct directional_light;

public:
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
  void set_shader(
      const std::function<void(Scene &Scene, int start_row, int start_col,
                               int block_row, int block_col)> &shader);
  Eigen::Vector3f get_eye_pos() const;
  Eigen::Vector3f get_view_dir() const;
  float get_zNear() const;
  float get_zFar() const;
  int get_width() const;
  int get_height() const;
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

private:
  Eigen::Vector3f eye_pos;
  Eigen::Vector3f view_dir;
  float zNear;
  float zFar;
  int width;
  int height;
  std::function<void(Scene &Scene, int start_row, int start_col, int block_row,
                     int block_col)>
      shader;
};