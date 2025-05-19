#include "light.hpp"
#include "Eigen/Core"
#include "Scene.hpp"
#include "global.hpp"

light::light() : pos(0.0f, 0.0f, 0.0f), intensity(0.0f, 0.0f, 0.0f) {}
light::light(const Eigen::Vector3f &pos, const Eigen::Vector3f &intensity)
    : pos(pos), intensity(intensity) {}

Eigen::Vector3f light::get_pos() const { return pos; }

void light::set_pos(const Eigen::Vector3f &pos) { this->pos = pos; }

Eigen::Vector3f light::get_intensity() const { return intensity; }

void light::set_intensity(const Eigen::Vector3f &intensity) {
  this->intensity = intensity;
}

void light::look_at(const Scene &scene) {}

bool light::in_shadow(const Eigen::Vector3f &point_pos,
                      const Eigen::Vector3f &normal) {
  return false;
}

float light::in_shadow_pcf(const Eigen::Vector3f &point_pos,
                           const Eigen::Vector3f &normal) {
  return 1.0f;
}

float light::in_shadow_pcss(const Eigen::Vector3f &point_pos,
                            const Eigen::Vector3f &normal) {
  return 1.0f;
}

void light::generate_penumbra_mask_block(
    const Scene &scene, std::vector<SHADOW_STATUS> &penumbra_mask,
    std::vector<float> &penumbra_mask_blur, int start_row, int start_col,
    int block_row, int block_col) {
  return;
}
