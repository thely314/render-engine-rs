#include "Model.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "assimp/mesh.h"
#include <Eigen/Dense>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
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

    if (mesh->mTextureCoords[0]) // 有纹理坐标
    {
      vertex.texture_coords = Eigen::Vector2f(mesh->mTextureCoords[0][i].x,
                                              mesh->mTextureCoords[0][i].y);
    } else {
      vertex.texture_coords = Eigen::Vector2f(0.0f, 0.0f);
    }

    vertices.push_back(vertex);
  }
  for (int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
      indices.push_back(face.mIndices[j]);
  }
  for (int i = 0; i < indices.size() - 2; ++i) {
    model.objects.push_back(new Triangle(vertices[indices[i]],
                                         vertices[indices[i + 1]],
                                         vertices[indices[i + 2]]));
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

int main() {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile("../models/utah_teapot.obj",
                        aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    std::cout << importer.GetErrorString() << '\n';
    return 1;
  }
  Model *model = new Model();
  processNode(scene->mRootNode, scene, *model);
  Scene Screen = Scene(800, 800);
  Screen.add_model(model);
  Screen.set_eye_pos({0, 1.0f, 5.0f});
  Screen.set_view_dir({0, 0, -1});
  Screen.set_zNear(-0.1f);
  Screen.set_zFar(1000.0f);
  Screen.start_render();
  Screen.save_to_file("output.png");
}