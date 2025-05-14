#pragma once
#include "Object.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "light.hpp"
#include <Triangle.hpp>
#include <array>
#include <global.hpp>
#include <memory>
#include <vector>

class Model : public Object {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Scene;
  friend struct light;
  friend struct spot_light;

public:
  static constexpr int TEXTURE_NUM = 4;
  enum TEXTURES {
    DIFFUSE_TEXTURE,
    SPECULAR_TEXTURE,
    NORMAL_TEXTURE,
    GLOW_TEXTURE
  };
  void set_pos(const Eigen::Vector3f &pos);
  Eigen::Vector3f get_pos() const;
  void set_scale(float rate);
  float get_scale() const;
  void set_texture(const std::shared_ptr<Texture> &texture, TEXTURES id);
  std::shared_ptr<Texture> get_texture(TEXTURES id) const;
  void modeling(const Eigen::Matrix<float, 4, 4> &modeling_matrix) override;
  void add(const std::shared_ptr<Model> &obj);
  void add(const Triangle &obj);
  ~Model();

private:
  void rasterization(const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene,
                     const Model &model) override;
  void rasterization_shadow_map(const Eigen::Matrix<float, 4, 4> &mvp,
                                spot_light &light) override;
  void rasterization_block(const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene,
                           const Model &model, int start_row, int start_col,
                           int block_row, int block_col) override;
  void rasterization_shadow_map_block(const Eigen::Matrix<float, 4, 4> &mvp,
                                      spot_light &light, int start_row,
                                      int start_col, int block_row,
                                      int block_col) override;
  void to_NDC(int width, int height);
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv, Model &parent) override;
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv);
  std::vector<std::shared_ptr<Model>> sub_models;
  std::vector<Triangle> triangles;
  std::vector<Triangle_rasterization> clip_triangles;
  Eigen::Vector3f pos = Eigen::Vector3f{0.0f, 0.0f, 0.0f};
  float scale = 1.0;
  std::array<std::shared_ptr<Texture>, TEXTURE_NUM> textures{};
};