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

Eigen::Vector3f
light::compute_world_light_dir(const Eigen::Vector3f &point_pos) const {
  return (point_pos - pos).normalized();
}

Eigen::Vector3f
light::compute_world_light_intensity(const Eigen::Vector3f &point_pos) const {
  float distance_square = (point_pos - pos).dot(point_pos - pos);
  return intensity / distance_square;
}

void light::look_at(const Scene &scene) {}

float light::in_shadow(const Eigen::Vector3f &point_pos,
                       const Eigen::Vector3f &normal,
                       SHADOW_METHOD shadow_method) const {
  return 1.0f;
}

bool light::in_penumbra_mask(int x, int y) const { return false; }

float light::in_shadow_direct(const Eigen::Vector3f &point_pos,
                              const Eigen::Vector3f &normal) const {
  return 1.0f;
}

float light::in_shadow_pcf(const Eigen::Vector3f &point_pos,
                           const Eigen::Vector3f &normal) const {
  return 1.0f;
}

float light::in_shadow_pcss(const Eigen::Vector3f &point_pos,
                            const Eigen::Vector3f &normal) const {
  return 1.0f;
}

void light::generate_penumbra_mask(const Scene &scene) {}

void light::box_blur_penumbra_mask(int radius) {}
