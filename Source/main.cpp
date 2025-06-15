#include "Eigen/Core"
#include "Model.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
#include "global.hpp"
#include "light.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
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
      for (auto &&light : scene.lights) {
        Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
        return_color += ambient;
        float visiblity;
        if (light->in_penumbra_mask(x, y)) {
          visiblity = light->in_shadow(pos, normal, light::PCSS);
        } else {
          visiblity = light->in_shadow(pos, normal, light::DIRECT);
        }
        if (visiblity < EPSILON) {
          continue;
        }
        Eigen::Vector3f eye2point = (pos - scene.get_eye_pos()).normalized();
        Eigen::Vector3f light2point = light->compute_world_light_dir(pos);
        Eigen::Vector3f light_intensity =
            light->compute_world_light_intensity(pos);
        Eigen::Vector3f half_dir = -(light2point + eye2point).normalized();
        Eigen::Vector3f diffuse = std::max(0.0f, normal.dot(-light2point)) *
                                  Kd.cwiseProduct(light_intensity);
        Eigen::Vector3f specular =
            powf(std::max(0.0f, half_dir.dot(normal)), 150) *
            Ks.cwiseProduct(light_intensity);
        return_color += visiblity * (diffuse + specular);
      }
      scene.frame_buffer[idx] = return_color;
    }
  }
}
int main() {
  auto start = std::chrono::system_clock::now();
  auto model = std::make_shared<Model>(
      //
      // "../models/tallbox.obj"
      "../models/diablo3/diablo3_pose.obj"
      //
  );

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
  // model->set_pos({0, -2.45f, 0});
  auto floor = std::make_shared<Model>("../models/floor.obj");
  // floor->set_scale(0.5f);
  floor->set_pos({0.0f, -2.45f, 0.0f});
  Scene my_scene = Scene(1024, 1024);
  my_scene.add_model(model);
  my_scene.add_model(floor);
  my_scene.set_eye_pos({0, 0, 7});
  my_scene.set_view_dir({0, 0, -1});
  my_scene.set_zNear(-0.1f);
  my_scene.set_zFar(-100.0f);
  // auto l1 = std::make_shared<SpotLight>();
  // l1->set_pos({10, 10, 10});
  // l1->set_intensity({250, 250, 250});
  // l1->set_aspect_ratio(1.0f);
  // l1->set_light_dir((model->get_pos() - l1->get_pos()).normalized());
  // l1->set_pcf_sample_accelerate_status(false);
  // l1->set_pcss_sample_accelerate_status(false);
  // l1->set_penumbra_mask_status(false);

  auto l2 = std::make_shared<DirectionalLight>();
  l2->set_pos({10, 10, 10});
  l2->set_intensity({1, 1, 1});
  l2->set_light_dir((model->get_pos() - l2->get_pos()).normalized());
  l2->set_pcf_sample_accelerate_status(true);
  l2->set_pcss_sample_accelerate_status(true);
  l2->set_penumbra_mask_status(true);
  // my_scene.add_light(l1);
  // my_scene.delete_model(floor);
  my_scene.add_light(l2);
  my_scene.set_shader(texture_shader);

  my_scene.start_render();
  my_scene.save_to_file("output.png");
  for (int i = 0; i != 36; ++i) {
    my_scene.start_render();
    my_scene.save_to_file(std::format("output{}.png", i + 1));
    model->rotate({0, 1, 0}, 10);
  }
  auto end = std::chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "花费了" << duration.count() << "ms\n";
}