#include "Model.hpp"
#include "Eigen/Core"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "light.hpp"
#include <memory>
Model::Model() : pos({0.0f, 0.0f, 0.0f}), scale(1.0f) {}
Model::Model(const char *model_path, Eigen::Vector3f default_color)
    : pos({0.0f, 0.0f, 0.0f}), scale(1.0f) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      model_path, aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    fprintf(stderr, "%s\n", importer.GetErrorString());
  } else {
    processNode(scene->mRootNode, scene, default_color);
  }
}
void Model::load(const char *model_path, Eigen::Vector3f default_color) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      model_path, aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    fprintf(stderr, "%s\n", importer.GetErrorString());
  } else {
    triangles.clear();
    clip_triangles.clear();
    pos = {0.0f, 0.0f, 0.0f};
    scale = 1.0f;
    processNode(scene->mRootNode, scene, default_color);
  }
}
void Model::set_pos(const Eigen::Vector3f &pos) {
  Eigen::Vector3f movement = pos - this->pos;
  this->pos = pos;
  Eigen::Matrix<float, 4, 4> modeling_matrix;
  modeling_matrix << 1.0f, 0.0f, 0.0f, movement.x(), 0.0f, 1.0f, 0.0f,
      movement.y(), 0.0f, 0.0f, 1.0f, movement.z(), 0.0f, 0.0f, 0.0f, 1.0f;
  for (auto &&obj : sub_models) {
    obj->modeling(modeling_matrix);
  }
  for (auto &&obj : triangles) {
    obj.modeling(modeling_matrix);
  }
}

Eigen::Vector3f Model::get_pos() const { return pos; }

void Model::set_scale(float rate) {
  float scale_rate = rate / scale;
  scale = rate;
  Eigen::Matrix<float, 4, 4> scale_matrix;
  scale_matrix << scale_rate, 0.0f, 0.0f, 0.0f, 0.0f, scale_rate, 0.0f, 0.0f,
      0.0f, 0.0f, scale_rate, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f;
  for (auto &&obj : sub_models) {
    obj->modeling(scale_matrix);
  }
  for (auto &&obj : triangles) {
    obj.modeling(scale_matrix);
  }
}

float Model::get_scale() const { return scale; }

void Model::set_texture(const std::shared_ptr<Texture> &texture,
                        Model::TEXTURES id) {
  textures[id] = texture;
}

std::shared_ptr<Texture> Model::get_texture(Model::TEXTURES id) const {
  return textures[id];
}

void Model::modeling(const Eigen::Matrix<float, 4, 4> &matrix) {
  for (auto &&obj : sub_models) {
    obj->modeling(matrix);
  }
  for (auto &&obj : triangles) {
    obj.modeling(matrix);
  }
}

void Model::add(const std::shared_ptr<Model> &obj) {
  sub_models.push_back(obj);
}
void Model::add(const Triangle &obj) { triangles.push_back(obj); }

Model::~Model() {}

void Model::rasterization_block(Scene &scene, const Model &model, int start_row,
                                int start_col, int block_row, int block_col) {
  for (auto &&obj : sub_models) {
    obj->rasterization_block(scene, *this, start_row, start_col, block_row,
                             block_col);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_block(scene, *this, start_row, start_col, block_row,
                            block_col);
  }
}
void Model::rasterization_shadow_map_block(spot_light &light, int start_row,
                                           int start_col, int block_row,
                                           int block_col) {
  for (auto &&obj : sub_models) {
    obj->rasterization_shadow_map_block(light, start_row, start_col, block_row,
                                        block_col);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_shadow_map_block(light, start_row, start_col, block_row,
                                       block_col);
  }
}

void Model::rasterization_shadow_map_block(directional_light &light,
                                           int start_row, int start_col,
                                           int block_row, int block_col) {
  for (auto &&obj : sub_models) {
    obj->rasterization_shadow_map_block(light, start_row, start_col, block_row,
                                        block_col);
  }
  for (auto &&obj : clip_triangles) {
    obj.rasterization_shadow_map_block(light, start_row, start_col, block_row,
                                       block_col);
  }
}

void Model::to_NDC(int width, int height) {
  for (auto &&obj : sub_models) {
    obj->to_NDC(width, height);
  }
  for (auto &&obj : clip_triangles) {
    obj.to_NDC(width, height);
  }
}

void Model::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                 const Eigen::Matrix<float, 4, 4> &mv, Model &parent) {
  // TODO:上锁
  clip_triangles.clear();
  for (auto &&obj : sub_models) {
    obj->clip(mvp, mv, *this);
  }
  for (auto &&obj : triangles) {
    obj.clip(mvp, mv, *this);
  }
}
void Model::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                 const Eigen::Matrix<float, 4, 4> &mv) {
  clip_triangles.clear();
  for (auto &&obj : sub_models) {
    obj->clip(mvp, mv, *this);
  }
  for (auto &&obj : triangles) {
    obj.clip(mvp, mv, *this);
  }
}

void Model::processNode(aiNode *node, const aiScene *scene,
                        Eigen::Vector3f default_color) {
  for (int i = 0; i != node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    this->processMesh(mesh, default_color);
  }
  for (int i = 0; i != node->mNumChildren; i++) {
    this->processNode(node->mChildren[i], scene, default_color);
  }
}
void Model::processMesh(aiMesh *mesh, Eigen::Vector3f default_color) {
  std::vector<Vertex> vertices;
  std::vector<int> indices;
  for (int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;
    vertex.pos = Eigen::Vector3f(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                 mesh->mVertices[i].z);

    if (mesh->HasNormals()) {
      vertex.normal = Eigen::Vector3f(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                      mesh->mNormals[i].z);
    }

    if (mesh->HasTextureCoords(0)) {
      vertex.texture_coords = Eigen::Vector2f(mesh->mTextureCoords[0][i].x,
                                              mesh->mTextureCoords[0][i].y);
    } else {
      vertex.texture_coords = Eigen::Vector2f(0.0f, 0.0f);
    }
    if (mesh->HasVertexColors(0)) {
      vertex.color = Eigen::Vector3f(
          mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b);
    } else {
      vertex.color = default_color;
    }
    vertices.push_back(vertex);
  }
  for (int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    if (face.mNumIndices == 3) {
      const Vertex &v0 = vertices[face.mIndices[0]];
      const Vertex &v1 = vertices[face.mIndices[1]];
      const Vertex &v2 = vertices[face.mIndices[2]];
      this->add(Triangle(v0, v1, v2));
    }
  }
}
void Model::flush() {
  triangles.clear();
  triangles.shrink_to_fit();
}