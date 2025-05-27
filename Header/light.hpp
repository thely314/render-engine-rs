#pragma once
#include <Eigen/Dense>
#include <vector>

constexpr float fibonacci_clump_exponent = 1.0f;
constexpr float fibonacci_spiral_direction[64][2]{
    {1.000000f, 0.000000f},   {-0.737369f, 0.675490f},
    {0.087426f, -0.996171f},  {0.608439f, 0.793601f},
    {-0.984713f, -0.174182f}, {0.843755f, -0.536728f},
    {-0.259604f, 0.965715f},  {-0.460907f, -0.887448f},
    {0.939321f, 0.343039f},   {-0.924346f, 0.381556f},
    {0.423846f, -0.905734f},  {0.299284f, 0.954164f},
    {-0.865211f, -0.501408f}, {0.976676f, -0.214719f},
    {-0.575129f, 0.818062f},  {-0.128511f, -0.991708f},
    {0.764649f, 0.644447f},   {-0.999146f, 0.041318f},
    {0.708829f, -0.705380f},  {-0.046191f, 0.998933f},
    {-0.640709f, -0.767784f}, {0.991069f, 0.133347f},
    {-0.820858f, 0.571132f},  {0.219481f, -0.975617f},
    {0.497181f, 0.867647f},   {-0.952693f, -0.303935f},
    {0.907791f, -0.419423f},  {-0.386061f, 0.922473f},
    {-0.338452f, -0.940984f}, {0.885189f, 0.465231f},
    {-0.966970f, 0.254890f},  {0.540838f, -0.841127f},
    {0.169376f, 0.985551f},   {-0.790623f, -0.612303f},
    {0.996586f, -0.082565f},  {-0.679079f, 0.734065f},
    {0.004878f, -0.999988f},  {0.671885f, 0.740655f},
    {-0.995733f, -0.092284f}, {0.796559f, -0.604560f},
    {-0.178984f, 0.983852f},  {-0.532606f, -0.846364f},
    {0.964437f, 0.264312f},   {-0.889686f, 0.456572f},
    {0.347617f, -0.937637f},  {0.377043f, 0.926196f},
    {-0.903656f, -0.428259f}, {0.955613f, -0.294626f},
    {-0.505622f, 0.862755f},  {-0.209952f, -0.977712f},
    {0.815247f, 0.579113f},   {-0.992323f, 0.123671f},
    {0.648169f, -0.761496f},  {0.036443f, 0.999336f},
    {-0.701914f, -0.712262f}, {0.998695f, 0.051064f},
    {-0.770900f, 0.636956f},  {0.138180f, -0.990407f},
    {0.567121f, 0.823635f},   {-0.974534f, -0.224238f},
    {0.870062f, -0.492942f},  {-0.308579f, 0.951199f},
    {-0.414989f, -0.909826f}, {0.920579f, 0.390557f},
};
// 斐波那契圆盘分布,全部为方向向量
// 出于提高采样质量的目的，可以以帧数做种生成一个随机数，然后把分布按照随机数的角度旋转
// 这种做法被称为temporal jitter,
// 原理是当两帧的时间差很小时，人眼会把阴影叠起来，从而模拟了更高采样数的效果
//  但是由于软光栅帧率太差，用不了这种方法，更多的优化方案还是等学了DX12再做吧
inline Eigen::Vector2f compute_fibonacci_spiral_disk_sample_uniform(
    int sample_index, float sample_count_inverse, float clumpExponent,
    float sample_dist_norm) {
  sample_dist_norm =
      powf((float)sample_index * sample_count_inverse, 0.5f * clumpExponent);
  return sample_dist_norm *
         Eigen::Vector2f{fibonacci_spiral_direction[sample_index][0],
                         fibonacci_spiral_direction[sample_index][1]};
}
struct Vertex;
struct Vertex_rasterization;
struct Scene;
class light {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct spot_light;
  friend struct Scene;

public:
  enum SHADOW_METHOD { DIRECT, PCF, PCSS };
  light();
  light(const Eigen::Vector3f &pos, const Eigen::Vector3f &intensity);
  Eigen::Vector3f get_pos() const;
  void set_pos(const Eigen::Vector3f &pos);
  Eigen::Vector3f get_intensity() const;
  void set_intensity(const Eigen::Vector3f &intensity);
  virtual void look_at(const Scene &);
  virtual float in_shadow(const Eigen::Vector3f &point_pos,
                          const Eigen::Vector3f &normal,
                          SHADOW_METHOD shadow_method) const;
  virtual bool in_penumbra_mask(int x, int y);
  virtual ~light() = default;

protected:
  virtual float in_shadow_direct(const Eigen::Vector3f &point_pos,
                                 const Eigen::Vector3f &normal) const;
  virtual float in_shadow_pcf(const Eigen::Vector3f &point_pos,
                              const Eigen::Vector3f &normal) const;
  virtual float in_shadow_pcss(const Eigen::Vector3f &point_pos,
                               const Eigen::Vector3f &normal) const;
  virtual void generate_penumbra_mask(const Scene &scene);
  virtual void box_blur_penumbra_mask(int radius);
  Eigen::Vector3f pos;
  Eigen::Vector3f intensity;
};
class spot_light : public light {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct Scene;

public:
  spot_light();
  Eigen::Vector3f get_light_dir() const;
  void set_light_dir(const Eigen::Vector3f &dir);
  float get_fov() const;
  void set_fov(float fov);
  float get_aspect_ratio() const;
  void set_aspect_ratio(float aspect_ratio);
  float get_zNear() const;
  void set_zNear(float zNear);
  float get_zFar() const;
  void set_zFar(float zFar);
  int get_width() const;
  void set_width(int width);
  int get_height() const;
  void set_height(int height);
  bool get_shadow_status() const;
  void set_shadow_status(bool status);
  bool get_pcf_sample_accelerate_status() const;
  void set_pcf_sample_accelerate_status(bool status);
  bool get_pcss_sample_accelerate_status() const;
  void set_pcss_sample_accelerate_status(bool status);
  bool get_penumbra_mask_status() const;
  void set_penumbra_mask_status(bool status);

private:
  Eigen::Vector3f light_dir;
  float fov;
  float aspect_ratio;
  float zNear;
  float zFar;
  float light_size;
  float fov_factor;
  float pixel_radius;
  int zbuffer_width;
  int zbuffer_height;
  int penumbra_mask_width;
  int penumbra_mask_height;
  bool enable_shadow;
  bool enable_pcf_sample_accelerate;
  bool enable_pcss_sample_accelerate;
  bool enable_penumbra_mask;
  Eigen::Matrix<float, 4, 4> mvp;
  Eigen::Matrix<float, 4, 4> mv;
  std::vector<float> z_buffer;
  std::vector<float> penumbra_mask;
  int get_index(int x, int y) const;
  int get_penumbra_mask_index(int x, int y) const;
  void look_at(const Scene &) override;
  float in_shadow(const Eigen::Vector3f &point_pos,
                  const Eigen::Vector3f &normal,
                  SHADOW_METHOD shadow_method) const override;
  bool in_penumbra_mask(int x, int y) override;
  float in_shadow_direct(const Eigen::Vector3f &point_pos,
                         const Eigen::Vector3f &normal) const override;
  float in_shadow_pcf(const Eigen::Vector3f &point_pos,
                      const Eigen::Vector3f &normal) const override;
  float in_shadow_pcss(const Eigen::Vector3f &point_pos,
                       const Eigen::Vector3f &normal) const override;
  void generate_penumbra_mask(const Scene &scene) override;
  void generate_penumbra_mask_block(const Scene &scene, int start_row,
                                    int start_col, int block_row,
                                    int block_col);
  void box_blur_penumbra_mask(int radius) override;
};
class directional_light : public light {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct Scene;

public:
  directional_light();
  Eigen::Vector3f get_light_dir() const;
  void set_light_dir(const Eigen::Vector3f &dir);
  float get_view_width() const;
  void set_view_width(float view_width);
  float get_view_height() const;
  void set_view_height(float view_height);
  float get_zNear() const;
  void set_zNear(float zNear);
  float get_zFar() const;
  void set_zFar(float zFar);
  int get_width() const;
  void set_width(int width);
  int get_height() const;
  void set_height(int height);
  bool get_shadow_status() const;
  void set_shadow_status(bool status);
  bool get_pcf_sample_accelerate_status() const;
  void set_pcf_sample_accelerate_status(bool status);
  bool get_pcss_sample_accelerate_status() const;
  void set_pcss_sample_accelerate_status(bool status);
  bool get_penumbra_mask_status() const;
  void set_penumbra_mask_status(bool status);

private:
  Eigen::Vector3f light_dir;
  float view_width;
  float view_height;
  float zNear;
  float zFar;
  float angular_diameter;
  // 方向光是用来模拟太阳光的
  // light_dir全程不变的原因是我们认为光源实际位置距离实际场景很远很远，所以可以忽略掉这个变化
  // 既然如此，light_size和光源到着色点的距离的变化也是极其微小的，我们引入方位角来描述它
  float pixel_radius;
  int zbuffer_width;
  int zbuffer_height;
  int penumbra_mask_width;
  int penumbra_mask_height;
  bool enable_shadow;
  bool enable_pcf_sample_accelerate;
  bool enable_pcss_sample_accelerate;
  bool enable_penumbra_mask;
  Eigen::Matrix<float, 4, 4> mvp;
  Eigen::Matrix<float, 4, 4> mv;
  std::vector<float> z_buffer;
  std::vector<float> penumbra_mask;
  int get_index(int x, int y) const;
  int get_penumbra_mask_index(int x, int y) const;
  void look_at(const Scene &) override;
  bool in_penumbra_mask(int x, int y) override;
  float in_shadow(const Eigen::Vector3f &point_pos,
                  const Eigen::Vector3f &normal,
                  SHADOW_METHOD shadow_method) const override;
  float in_shadow_direct(const Eigen::Vector3f &point_pos,
                         const Eigen::Vector3f &normal) const override;
  float in_shadow_pcf(const Eigen::Vector3f &point_pos,
                      const Eigen::Vector3f &normal) const override;
  float in_shadow_pcss(const Eigen::Vector3f &point_pos,
                       const Eigen::Vector3f &normal) const override;
  void generate_penumbra_mask(const Scene &scene) override;
  void generate_penumbra_mask_block(const Scene &scene, int start_row,
                                    int start_col, int block_row,
                                    int block_col);
  void box_blur_penumbra_mask(int radius) override;
};