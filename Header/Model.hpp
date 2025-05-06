#pragma once
#include "Eigen/Core"
#include "Texture.hpp"
#include <Triangle.hpp>
#include <array>
#include <global.hpp>
#include <memory>
#include <vector>

struct Model : public Object {
  static constexpr int TEXTURE_NUM = 4;
  enum { DIFFUSE_TEXTURE, SPECULAR_TEXTURE, NORMAL_TEXTURE, GLOW_TEXTURE };
  void rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                     const Eigen::Matrix<float, 3, 3> &normal_mvp, Scene &scene,
                     const Model &model) override;
  void set_pos(const Eigen::Vector3f &pos);
  void move(const Eigen::Matrix<float, 4, 4> &modeling_matrix) override;
  void set_scale(float rate);
  void set_diffuse_texture(const std::shared_ptr<Texture> &texture);
  void set_specular_texture(const std::shared_ptr<Texture> &texture);
  void set_normal_texture(const std::shared_ptr<Texture> &texture);
  void set_glow_texture(const std::shared_ptr<Texture> &texture);
  void generate_shadowmap(const Eigen::Matrix<float, 4, 4> &mvp,
                          const Eigen::Matrix<float, 3, 3> &normal_mvp,
                          spot_light &light) override;
  ~Model();
  std::vector<Object *> objects;
  Eigen::Vector3f pos = Eigen::Vector3f{0.0f, 0.0f, 0.0f};
  float scale = 1.0;
  std::array<std::shared_ptr<Texture>, TEXTURE_NUM> textures{};
};