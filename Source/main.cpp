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
Eigen::Vector3f phong_shader(const Vertex &point, const Scene &scene,
                             const Model &model) {
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
Eigen::Vector3f texture_shader(const Vertex &point, const Scene &scene,
                               const Model &model) {
  Eigen::Vector3f Ks{0.8f, 0.8f, 0.8f};
  Eigen::Vector3f Kd;
  if (model.textures[Model::TEXTURE] == nullptr) {
    std::cout << "Error:Texture is nullptr\n";
    Kd = point.color;
  } else {
    Kd = model.textures[Model::TEXTURE]->get_color(point.texture_coords.x(),
                                                   point.texture_coords.y());
    // std::cout << Kd << '\n';
  }
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
}
int main() {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile("../models/spot_triangulated_good.obj",
                        aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    std::cout << importer.GetErrorString() << '\n';
    return 1;
  }
  Model *model = new Model();
  processNode(scene->mRootNode, scene, *model);
  std::shared_ptr<Texture> texture =
      std::make_shared<Texture>("../models/spot_texture.png");
  model->set_texture(texture);
  Scene my_scene = Scene(800, 800);
  model->set_scale(2.5f);
  model->move(get_model_matrix({0, 1, 0}, 140, {0.0, 0.0, 0.0}));
  my_scene.add_model(model);
  my_scene.set_eye_pos({0.0f, 0.0f, 10.0f});
  my_scene.set_view_dir({0, 0, -1});
  my_scene.set_zNear(-0.1f);
  my_scene.set_zFar(-50.0f);
  my_scene.lights.push_back({{20, 20, 20}, {500, 500, 500}});
  my_scene.lights.push_back({{-20, 20, 0}, {500, 500, 500}});
  my_scene.shader = texture_shader;
  my_scene.start_render();
  my_scene.save_to_file("output.png");
}