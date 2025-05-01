#include "Eigen/Core"
#include "Model.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "assimp/mesh.h"
#include "global.hpp"
#include <Eigen/Dense>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
void processMesh(aiMesh *mesh, const aiScene *scene, Model &model) {
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

    if (mesh->mTextureCoords[0]) {
      vertex.texture_coords = Eigen::Vector2f(mesh->mTextureCoords[0][i].x,
                                              mesh->mTextureCoords[0][i].y);
    } else {
      vertex.texture_coords = Eigen::Vector2f(0.0f, 0.0f);
    }

    vertices.push_back(vertex);
  }
  for (int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    if (face.mNumIndices == 3) {
      const Vertex &v0 = vertices[face.mIndices[0]];
      const Vertex &v1 = vertices[face.mIndices[1]];
      const Vertex &v2 = vertices[face.mIndices[2]];
      model.objects.push_back(new Triangle(v0, v1, v2));
    }
  }
}
void processNode(aiNode *node, const aiScene *scene, Model &model) {
  for (int i = 0; i != node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    processMesh(mesh, scene, model);
  }
  for (int i = 0; i != node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene, model);
  }
}
Eigen::Vector3f phong_shader(Vertex &point, const Scene &scene,
                             const Model &model, const Eigen::Vector3f &tangent,
                             const Eigen::Vector3f &binormal) {
  Eigen::Vector3f Ks{0.8f, 0.8f, 0.8f};
  Eigen::Vector3f Kd = point.color;
  Eigen::Vector3f Ka{0.005, 0.005, 0.005};
  Eigen::Vector3f ambient_intensity = {10, 10, 10};
  Eigen::Vector3f result_color{0.0f, 0.0f, 0.0f};
  std::vector<light> lights = {{{20, 20, 20}, {500, 500, 500}},
                               {{-20, 20, 0}, {500, 500, 500}}};
  for (auto &&light : lights) {
    Eigen::Vector3f eye_dir = (point.pos - scene.eye_pos).normalized();
    Eigen::Vector3f light_dir = light.pos - point.pos;
    float light_distance_square = light_dir.dot(light_dir);
    light_dir = light_dir.normalized();
    Eigen::Vector3f half_dir = (light_dir - eye_dir).normalized();
    Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
    Eigen::Vector3f diffuse =
        Kd.cwiseProduct(light.intensity / light_distance_square *
                        std::max(0.0f, point.normal.dot(light_dir)));
    Eigen::Vector3f specular =
        Ks.cwiseProduct(light.intensity / light_distance_square *
                        pow(std::max(0.0f, point.normal.dot(half_dir)), 150));
    result_color = result_color + ambient + diffuse + specular;
  }
  return result_color;
};
Eigen::Vector3f texture_shader(Vertex &point, const Scene &scene,
                               const Model &model,
                               const Eigen::Vector3f &tangent,
                               const Eigen::Vector3f &binormal) {
  Eigen::Vector3f Ks;
  Eigen::Vector3f Kd;
  if (model.textures[Model::DIFFUSE_TEXTURE] == nullptr) {
    Kd = point.color;
  } else {
    Kd = model.textures[Model::DIFFUSE_TEXTURE]->get_color(
        point.texture_coords.x(), point.texture_coords.y());
  }
  if (model.textures[Model::SPECULAR_TEXTURE] == nullptr) {
    Ks = Eigen::Vector3f{0.8f, 0.8f, 0.8f};
  } else {
    Ks = model.textures[Model::SPECULAR_TEXTURE]->get_color(
        point.texture_coords.x(), point.texture_coords.y());
  }
  if (model.textures[Model::NORMAL_TEXTURE] != nullptr) {
    Eigen::Vector3f TBN_normal =
        (2.0f * model.textures[Model::NORMAL_TEXTURE]->get_color(
                    point.texture_coords.x(), point.texture_coords.y()) -
         Eigen::Vector3f{1.0f, 1.0f, 1.0f})
            .normalized();
    Eigen::Vector3f tangent_orthogonal =
        (tangent - tangent.dot(point.normal) * point.normal).normalized();
    Eigen::Vector3f binormal_orthogonal =
        point.normal.cross(tangent_orthogonal).normalized();
    if (binormal_orthogonal.dot(binormal) <= 0) {
      binormal_orthogonal = -binormal_orthogonal;
    }
    Eigen::Matrix<float, 3, 3> TBN;
    TBN << tangent_orthogonal, binormal_orthogonal, point.normal;
    point.normal = (TBN * TBN_normal).normalized();
  }
  Eigen::Vector3f Ka{0.005, 0.005, 0.005};
  Eigen::Vector3f ambient_intensity = {10, 10, 10};
  Eigen::Vector3f result_color{0.0f, 0.0f, 0.0f};
  if (model.textures[Model::GLOW_TEXTURE] != nullptr) {
    result_color = model.textures[Model::GLOW_TEXTURE]->get_color(
        point.texture_coords.x(), point.texture_coords.y());
  }
  for (auto &&light : scene.lights) {
    Eigen::Vector3f eye_dir = (point.pos - scene.eye_pos).normalized();
    Eigen::Vector3f light_dir = light.pos - point.pos;
    float light_distance_square = light_dir.dot(light_dir);
    light_dir = light_dir.normalized();
    Eigen::Vector3f half_dir = (light_dir - eye_dir).normalized();
    Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
    Eigen::Vector3f diffuse =
        Kd.cwiseProduct(light.intensity / light_distance_square *
                        std::max(0.0f, point.normal.dot(light_dir)));
    Eigen::Vector3f specular =
        Ks.cwiseProduct(light.intensity / light_distance_square *
                        pow(std::max(0.0f, point.normal.dot(half_dir)), 150));
    result_color = result_color + ambient + diffuse + specular;
  }
  return result_color;
}
int main() {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile("../models/diablo3/diablo3_pose.obj",
                        aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    std::cout << importer.GetErrorString() << '\n';
    return 1;
  }
  Model *model = new Model();
  processNode(scene->mRootNode, scene, *model);
  std::shared_ptr<Texture> diffuse_texture =
      std::make_shared<Texture>("../models/diablo3/diablo3_pose_diffuse.tga");
  model->set_diffuse_texture(diffuse_texture);
  std::shared_ptr<Texture> specular_texture =
      std::make_shared<Texture>("../models/diablo3/diablo3_pose_spec.tga");
  model->set_specular_texture(specular_texture);
  std::shared_ptr<Texture> normal_texture = std::make_shared<Texture>(
      "../models/diablo3/diablo3_pose_nm_tangent.tga");
  model->set_normal_texture(normal_texture);
  std::shared_ptr<Texture> glow_texture =
      std::make_shared<Texture>("../models/diablo3/diablo3_pose_glow.tga");
  model->set_glow_texture(glow_texture);
  Scene my_scene = Scene(1024, 1024);
  model->set_scale(2.5f);
  my_scene.add_model(model);
  my_scene.set_eye_pos({0.0f, 0.0f, 7.0f});
  my_scene.set_view_dir({0, 0, -1});
  my_scene.set_zNear(-0.1f);
  my_scene.set_zFar(-50.0f);
  my_scene.lights.push_back({{20, 20, 20}, {500, 500, 500}});
  my_scene.lights.push_back({{-20, 20, 0}, {500, 500, 500}});
  my_scene.shader = texture_shader;
  my_scene.start_render();
  my_scene.save_to_file("output.png");
}