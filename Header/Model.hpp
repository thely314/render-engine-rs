#pragma once
#include "Eigen/Core"
#include "Object.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
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
  void move(const Eigen::Matrix<float, 4, 4> &modeling_matrix) override;
  void add(Object *obj);
  ~Model();

private:
  void rasterization(const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene,
                     const Model &model) override;
  void rasterization_shadow_map(const Eigen::Matrix<float, 4, 4> &mvp,
                                spot_light &light) override;
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            std::vector<Object *> &objects) override;
  void clip(const Eigen::Matrix<float, 4, 4> &mvp);
  std::vector<Object *> objects;
  std::vector<Object *> clip_objects;
  Eigen::Vector3f pos = Eigen::Vector3f{0.0f, 0.0f, 0.0f};
  float scale = 1.0;
  std::array<std::shared_ptr<Texture>, TEXTURE_NUM> textures{};
};