#include "Eigen/Core"
#include "Model.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "light.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <format>
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
      for (int i = 0; i != scene.lights.size(); ++i) {
        Eigen::Vector3f ambient = Ka.cwiseProduct(ambient_intensity);
        return_color += ambient;
        float shadow_result = 1.0f;
        if (scene.lights[i]->in_penumbra_mask(x, y)) {
          shadow_result = scene.lights[i]->in_shadow(pos, normal, light::PCSS);
          // return_color = {1.0f, 1.0f, 1.0f};
        } else {
          // return_color = {0.0f, 0.0f, 0.0f};
          shadow_result =
              scene.lights[i]->in_shadow(pos, normal, light::DIRECT);
        }
        // break;
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
  auto l1 = std::make_shared<spot_light>();
  l1->set_pos({10, 10, 10});
  l1->set_intensity({250, 250, 250});
  l1->set_aspect_ratio(1.0f);
  l1->set_light_dir((model->get_pos() - l1->get_pos()).normalized());
  l1->set_pcf_sample_accelerate_status(true);
  l1->set_pcss_sample_accelerate_status(true);
  l1->set_penumbra_mask_status(true);

  auto l2 = std::make_shared<directional_light>();
  l2->set_pos({10, 10, 10});
  l2->set_intensity({250, 250, 250});
  l2->set_light_dir((model->get_pos() - l2->get_pos()).normalized());
  l2->set_pcf_sample_accelerate_status(false);
  l2->set_pcss_sample_accelerate_status(false);
  l2->set_penumbra_mask_status(true);
  my_scene.add_light(l2);
  // my_scene.delete_model(floor);
  // my_scene.add_light(l2);
  my_scene.set_shader(texture_shader);

  // model->modeling(  get_modeling_matrix({0, 1, 0}, 50, {0, 0, 0}));
  my_scene.start_render();
  my_scene.save_to_file("output.png");
  // for (int i = 0; i != 36; ++i) {
  //   my_scene.start_render();
  //   my_scene.save_to_file(std::format("output{}.png", i + 1));
  //   model->modeling(get_modeling_matrix({0, 1, 0}, 10, {0, 0, 0}));
  // }
}