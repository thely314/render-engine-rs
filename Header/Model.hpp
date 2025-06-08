#pragma once
#include "Eigen/Core"
#include "Triangle.hpp"
#include <array>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <memory>
#include <vector>
struct Scene;
struct Texture;
class Model {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Scene;
  friend struct light;
  friend struct spot_light;
  friend struct directional_light;

public:
  static constexpr int TEXTURE_NUM = 4;
  enum TEXTURES {
    DIFFUSE_TEXTURE,
    SPECULAR_TEXTURE,
    NORMAL_TEXTURE,
    GLOW_TEXTURE
  };
  Model();
  Model(const char *model_path,
        Eigen::Vector3f default_color = {0.5f, 0.5f, 0.5f});
  void load(const char *model_path,
            Eigen::Vector3f default_color = {0.5f, 0.5f, 0.5f});
  void set_pos(const Eigen::Vector3f pos);
  Eigen::Vector3f get_pos() const;
  void rotate(Eigen::Vector3f axis, float angle);
  void set_scale(float rate);
  float get_scale() const;
  void set_texture(const std::shared_ptr<Texture> &texture, TEXTURES id);
  const std::shared_ptr<Texture> &get_texture(TEXTURES id) const;
  void modeling(const Eigen::Matrix<float, 4, 4> &modeling_matrix);
  void add(const std::shared_ptr<Model> &obj);
  void add(const Triangle &obj);
  void flush();
  ~Model();

private:
  void rasterization_block(Scene &scene, const Model &model, int start_row,
                           int start_col, int block_row, int block_col);
  template <bool IsProjection>
  void rasterization_shadow_map_block(
      std::vector<float> &z_buffer, int start_row, int start_col, int block_row,
      int block_col,
      const std::function<float(float z, float w)> &depth_transformer,
      const std::function<int(int x, int y)> &get_index) {
    for (auto &&obj : sub_models) {
      obj->rasterization_shadow_map_block<IsProjection>(
          z_buffer, start_row, start_col, block_row, block_col,
          depth_transformer, get_index);
    }
    for (auto &&obj : clip_triangles) {
      obj.template rasterization_shadow_map_block<IsProjection>(
          z_buffer, start_row, start_col, block_row, block_col,
          depth_transformer, get_index);
    }
  }
  void to_NDC(int width, int height);
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv, Model &parent);
  void clip(const Eigen::Matrix<float, 4, 4> &mvp,
            const Eigen::Matrix<float, 4, 4> &mv);
  void processNode(aiNode *node, const aiScene *scene,
                   Eigen::Vector3f default_color);
  void processMesh(aiMesh *mesh, Eigen::Vector3f default_color);
  std::vector<std::shared_ptr<Model>> sub_models;
  std::vector<Triangle> triangles;
  std::vector<Triangle_rasterization> clip_triangles;
  Eigen::Vector3f pos;
  float scale;
  std::array<std::shared_ptr<Texture>, TEXTURE_NUM> textures{};
};