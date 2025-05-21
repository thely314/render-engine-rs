#include "Eigen/Core"
#include "Model.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "light.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <memory>
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
      model.add(Triangle(v0, v1, v2));
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
void texture_shader(Scene &scene, int start_row, int start_col, int block_row,
                    int block_col) {
  for (int y = start_row; y < start_row + block_row; ++y) {
    for (int x = start_col; x < start_col + block_col; ++x) {
      int idx = scene.get_index(x, y);
      if (scene.z_buffer[idx] == INFINITY) {
        continue;
      }
      Eigen::Vector3f pos = scene.pos_buffer[idx];
      Eigen::Vector3f normal = scene.normal_buffer[idx];
      Eigen::Vector3f Ka = scene.diffuse_buffer[idx];
      Eigen::Vector3f Kd = scene.diffuse_buffer[idx];
      Eigen::Vector3f Ks = scene.specular_buffer[idx];
      Eigen::Vector3f return_color = scene.glow_buffer[idx];
      Eigen::Vector3f ambient_intensity{0.05f, 0.05f, 0.05f};
      for (int i = 0; i != scene.lights.size(); ++i) {
        Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
        return_color += ambient;
        float shadow_result = 1.0f;
        // if (scene.penumbra_masks[i][scene.get_penumbra_mask_index(
        //         x / 4, y / 4)] == light::PENUMBRA) {
        //   return_color = {1.0f, 1.0f, 1.0f};
        // } else {
        //   return_color = {0.0f, 0.0f, 0.0f};
        // }
        // break;
        if (scene.get_penumbra_mask_status()) {
          light::SHADOW_STATUS shadow_status =
              scene.penumbra_masks[i]
                                  [scene.get_penumbra_mask_index(x / 4, y / 4)];
          switch (shadow_status) {
          case light::BRIGHT:
            break;
          case light::PENUMBRA: {
            switch (scene.get_shadow_method()) {
            case Scene::PCF:
              shadow_result = scene.lights[i]->in_shadow_pcf(pos, normal);
              break;
            case Scene::PCSS:
              shadow_result = scene.lights[i]->in_shadow_pcss(pos, normal);
              break;
            }
            break;
          }
          case light::SHADOW:
            shadow_result = 0.0f;
          }
          if (shadow_result < EPSILON) {
            continue;
          }
        } else {
          switch (scene.get_shadow_method()) {
          case Scene::PCF:
            shadow_result = scene.lights[i]->in_shadow_pcf(pos, normal);
            break;
          case Scene::PCSS:
            shadow_result = scene.lights[i]->in_shadow_pcss(pos, normal);
            break;
          }
        }
        Eigen::Vector3f eye_dir = (pos - scene.get_eye_pos()).normalized();
        Eigen::Vector3f light_dir = scene.lights[i]->get_pos() - pos;
        float light_distance_square = light_dir.dot(light_dir);
        light_dir = light_dir.normalized();
        Eigen::Vector3f half_dir = (light_dir - eye_dir).normalized();
        Eigen::Vector3f diffuse = Kd.cwiseProduct(
            scene.lights[i]->get_intensity() / light_distance_square *
            std::max(0.0f, normal.dot(light_dir)));
        Eigen::Vector3f specular = Ks.cwiseProduct(
            scene.lights[i]->get_intensity() / light_distance_square *
            pow(std::max(0.0f, normal.dot(half_dir)), 150));
        return_color += (diffuse + specular) * shadow_result;
      }
      scene.frame_buffer[idx] = return_color;
    }
  }
}
int main() {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      //
      // "../models/tallbox.obj",
      "../models/diablo3/diablo3_pose.obj",
      aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    fprintf(stderr, "%s\n", importer.GetErrorString());
    return 1;
  }
  auto model = std::make_shared<Model>();

  processNode(scene->mRootNode, scene, *model);

  std::shared_ptr<Texture> diffuse_texture =
      std::make_shared<Texture>("../models/diablo3/diablo3_pose_diffuse.tga");
  model->set_texture(diffuse_texture, Model::DIFFUSE_TEXTURE);

  std::shared_ptr<Texture> specular_texture =
      std::make_shared<Texture>("../models/diablo3/diablo3_pose_spec.tga");
  model->set_texture(specular_texture, Model::SPECULAR_TEXTURE);

  std::shared_ptr<Texture> normal_texture = std::make_shared<Texture>(
      "../models/diablo3/diablo3_pose_nm_tangent.tga");
  model->set_texture(normal_texture, Model::NORMAL_TEXTURE);

  std::shared_ptr<Texture> glow_texture =
      std::make_shared<Texture>("../models/diablo3/diablo3_pose_glow.tga");
  model->set_texture(glow_texture, Model::GLOW_TEXTURE);

  model->set_scale(2.5f);
  // model->set_pos({0.0f, -2.45f, 0.0f});
  Vertex floor_vertex[4];
  floor_vertex[0] = Vertex{{-10.0f, -2.45f, -10.0f},
                           {0.0f, 1.0f, 0.0f},
                           {0.5f, 0.5f, 0.5f},
                           {0.0f, 0.0f}};
  floor_vertex[1] = Vertex{{10.0f, -2.45f, 10.0f},
                           {0.0f, 1.0f, 0.0f},
                           {0.5f, 0.5f, 0.5f},
                           {0.0f, 0.0f}};
  floor_vertex[2] = Vertex{{10.0f, -2.45f, -10.0f},
                           {0.0f, 1.0f, 0.0f},
                           {0.5f, 0.5f, 0.5f},
                           {0.0f, 0.0f}};
  floor_vertex[3] = Vertex{{-10.0f, -2.45f, 10.0f},
                           {0.0f, 1.0f, 0.0f},
                           {0.5f, 0.5f, 0.5f},
                           {0.0f, 0.0f}};
  auto floor = std::make_shared<Model>();
  floor->add(Triangle(floor_vertex[0], floor_vertex[1], floor_vertex[2]));
  floor->add(Triangle(floor_vertex[0], floor_vertex[3], floor_vertex[1]));
  Scene my_scene = Scene(1024, 1024);
  my_scene.add_model(model);
  my_scene.add_model(floor);
  my_scene.set_eye_pos({0, 0, 7});
  my_scene.set_view_dir({0, 0, -1});
  my_scene.set_zNear(-0.1f);
  my_scene.set_zFar(-100.0f);
  auto l1 = std::make_shared<spot_light>();
  l1->set_pos({10, 10, 10});
  l1->set_intensity({250, 250, 250});
  l1->set_aspect_ratio(1.0f);
  l1->set_light_dir((model->get_pos() - l1->get_pos()).normalized());
  l1->set_pcf_sample_accelerate_status(true);
  l1->set_pcss_sample_accelerate_status(true);
  // auto l2 = std::make_shared<directional_light>();
  // l2->set_pos({10, 10, 10});
  // l2->set_intensity({250, 250, 250});
  // l2->set_light_dir((model->get_pos() - l2->get_pos()).normalized());
  // l2->set_pcf_sample_accelerate_status(true);
  // l2->set_pcss_sample_accelerate_status(true);
  my_scene.add_light(l1);
  // my_scene.add_light(l2);
  my_scene.set_shader(texture_shader);
  // l1->set_shadow_status(false);
  my_scene.set_penumbra_mask_status(false);
  my_scene.set_shadow_method(Scene::PCSS);
  // model->modeling(get_model_matrix({0, 1, 0}, 50, {0, 0, 0}));
  my_scene.start_render();
  my_scene.save_to_file("output.png");
  for (int i = 0; i != 36; ++i) {
    my_scene.start_render();
    my_scene.save_to_file(std::format("output{}.png", i + 1));
    model->modeling(get_model_matrix({0, 1, 0}, 10, {0, 0, 0}));
  }
}