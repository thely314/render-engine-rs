#include "Model.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "light.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <format>
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
Eigen::Vector3f phong_shader(Vertex_rasterization &point, const Scene &scene,
                             const Model &model, const Eigen::Vector3f &tangent,
                             const Eigen::Vector3f &binormal) {
  Eigen::Vector3f Ks{0.8f, 0.8f, 0.8f};
  Eigen::Vector3f Kd = point.color;
  Eigen::Vector3f Ka{0.005, 0.005, 0.005};
  Eigen::Vector3f ambient_intensity = {10, 10, 10};
  Eigen::Vector3f result_color{0.0f, 0.0f, 0.0f};
  for (auto &&light : scene.lights) {
    Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
    if (light->in_shadow(point)) {
      result_color += ambient;
      continue;
    }
    Eigen::Vector3f eye_dir = (point.pos - scene.get_eye_pos()).normalized();
    Eigen::Vector3f light_dir = light->get_pos() - point.pos;
    float light_distance_square = light_dir.dot(light_dir);
    light_dir = light_dir.normalized();
    Eigen::Vector3f half_dir = (light_dir - eye_dir).normalized();
    Eigen::Vector3f diffuse =
        Kd.cwiseProduct(light->get_intensity() / light_distance_square *
                        std::max(0.0f, point.normal.dot(light_dir)));
    Eigen::Vector3f specular =
        Ks.cwiseProduct(light->get_intensity() / light_distance_square *
                        pow(std::max(0.0f, point.normal.dot(half_dir)), 150));
    result_color += ambient + diffuse + specular;
  }
  return result_color;
};
Eigen::Vector3f texture_shader(Vertex_rasterization &point, const Scene &scene,
                               const Model &model,
                               const Eigen::Vector3f &tangent,
                               const Eigen::Vector3f &binormal) {
  Eigen::Vector3f Ks;
  Eigen::Vector3f Kd;
  if (model.get_texture(Model::DIFFUSE_TEXTURE) == nullptr) {
    Kd = point.color;
  } else {
    Kd = model.get_texture(Model::DIFFUSE_TEXTURE)
             ->get_color(point.texture_coords.x(), point.texture_coords.y());
  }
  if (model.get_texture(Model::SPECULAR_TEXTURE) == nullptr) {
    Ks = Eigen::Vector3f{0.8f, 0.8f, 0.8f};
  } else {
    Ks = model.get_texture(Model::SPECULAR_TEXTURE)
             ->get_color(point.texture_coords.x(), point.texture_coords.y());
  }
  if (model.get_texture(Model::NORMAL_TEXTURE) != nullptr) {
    Eigen::Vector3f TBN_normal =
        (2.0f * model.get_texture(Model::NORMAL_TEXTURE)
                    ->get_color(point.texture_coords.x(),
                                point.texture_coords.y()) -
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
  Eigen::Vector3f Ka{0.005f, 0.005f, 0.005f};
  Eigen::Vector3f ambient_intensity = {10, 10, 10};
  Eigen::Vector3f result_color{0.0f, 0.0f, 0.0f};
  if (model.get_texture(Model::GLOW_TEXTURE) != nullptr) {
    result_color =
        model.get_texture(Model::GLOW_TEXTURE)
            ->get_color(point.texture_coords.x(), point.texture_coords.y());
  }
  for (auto &&light : scene.lights) {
    Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
    float shadow_result = light->in_shadow_pcss(point);
    if (shadow_result < EPSILON) {
      continue;
    }
    Eigen::Vector3f eye_dir = (point.pos - scene.get_eye_pos()).normalized();
    Eigen::Vector3f light_dir = light->get_pos() - point.pos;
    float light_distance_square = light_dir.dot(light_dir);
    light_dir = light_dir.normalized();
    Eigen::Vector3f half_dir = (light_dir - eye_dir).normalized();
    Eigen::Vector3f diffuse =
        Kd.cwiseProduct(light->get_intensity() / light_distance_square *
                        std::max(0.0f, point.normal.dot(light_dir)));
    Eigen::Vector3f specular =
        Ks.cwiseProduct(light->get_intensity() / light_distance_square *
                        pow(std::max(0.0f, point.normal.dot(half_dir)), 150));
    result_color += (ambient + diffuse + specular) * shadow_result;
  }
  return result_color;
}
int main() {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      //
      "../models/tallbox.obj",
      // "../models/diablo3/diablo3_pose.obj",
      aiProcess_Triangulate | aiProcess_GenNormals);
  if (scene == nullptr) {
    fprintf(stderr, "%s\n", importer.GetErrorString());
    return 1;
  }
  auto model = std::make_shared<Model>();

  processNode(scene->mRootNode, scene, *model);

  // std::shared_ptr<Texture> diffuse_texture =
  //     std::make_shared<Texture>("../models/diablo3/diablo3_pose_diffuse.tga");
  // model->set_texture(diffuse_texture, Model::DIFFUSE_TEXTURE);

  // std::shared_ptr<Texture> specular_texture =
  //     std::make_shared<Texture>("../models/diablo3/diablo3_pose_spec.tga");
  // model->set_texture(specular_texture, Model::SPECULAR_TEXTURE);

  // std::shared_ptr<Texture> normal_texture = std::make_shared<Texture>(
  //     "../models/diablo3/diablo3_pose_nm_tangent.tga");
  // model->set_texture(normal_texture, Model::NORMAL_TEXTURE);

  // std::shared_ptr<Texture> glow_texture =
  //     std::make_shared<Texture>("../models/diablo3/diablo3_pose_glow.tga");
  // model->set_texture(glow_texture, Model::GLOW_TEXTURE);

  // model->set_scale(2.5f);
  model->set_pos({0.0f, -2.45f, 0.0f});
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
  Scene my_scene = Scene(2048, 2048);
  my_scene.add_model(model);
  my_scene.add_model(floor);
  // my_scene.set_eye_pos({-10.0f, 0.0f, -10.0f});
  // my_scene.set_view_dir(
  //     (model->get_pos() - my_scene.get_eye_pos()).normalized());
  // my_scene.set_view_dir(-my_scene.get_eye_pos().normalized());
  my_scene.set_eye_pos({0, 0, 7});
  my_scene.set_view_dir({0, 0, -1});
  my_scene.set_zNear(0.1f);
  my_scene.set_zFar(100.0f);
  // my_scene.set_width(2048);
  // my_scene.set_height(2048);
  auto l1 = std::make_shared<spot_light>();
  l1->set_pos({10, 10, 10});
  l1->set_intensity({250, 250, 250});
  l1->set_light_dir((model->get_pos() - l1->get_pos()).normalized());
  // auto l2 = std::make_shared<spot_light>(*l1);
  // l2->set_pos({-20, 20, 20});
  // l2->set_light_dir((model->get_pos() - l2->get_pos()).normalized());
  my_scene.add_light(l1);
  // my_scene.add_light(l2);
  my_scene.set_shader(texture_shader);
  // l1->set_shadow_status(false);
  l1->set_pcf_poisson_status(false);
  l1->set_pcss_poisson_status(false);
  my_scene.start_render();
  my_scene.save_to_file("output.png");
  // for (int i = 0; i != 36; ++i) {
  //   my_scene.start_render();
  //   my_scene.save_to_file(std::format("output{}.png", i + 1));
  //   model->modeling(get_model_matrix({0, 1, 0}, 10, {0, 0, 0}));
  // }
}