#pragma once
#include <Eigen/Dense>
#include <vector>
struct Vertex;
struct Vertex_rasterization;
struct Scene;
class light {
  friend struct Triangle;
  friend struct Triangle_rasterization;
  friend struct Model;
  friend struct spot_light;

public:
  enum SHADOW_STATUS { BRIGHT, PENUMBRA, SHADOW };
  light();
  light(const Eigen::Vector3f &pos, const Eigen::Vector3f &intensity);
  Eigen::Vector3f get_pos() const;
  void set_pos(const Eigen::Vector3f &pos);
  Eigen::Vector3f get_intensity() const;
  void set_intensity(const Eigen::Vector3f &intensity);
  virtual void look_at(const Scene &);
  virtual bool in_shadow(const Eigen::Vector3f &point_pos,
                         const Eigen::Vector3f &normal);
  virtual float in_shadow_pcf(const Eigen::Vector3f &point_pos,
                              const Eigen::Vector3f &normal);
  virtual float in_shadow_pcss(const Eigen::Vector3f &point_pos,
                               const Eigen::Vector3f &normal);
  virtual void generate_penumbra_mask_block(
      const Scene &scene, std::vector<SHADOW_STATUS> &penumbra_mask,
      std::vector<float> &penumbra_mask_blur, int start_row, int start_col,
      int block_row, int block_col);
  virtual ~light() = default;

protected:
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

private:
  Eigen::Vector3f light_dir;
  float fov;
  float aspect_ratio;
  float zNear;
  float zFar;
  float light_size;
  float fov_factor;
  int zbuffer_width;
  int zbuffer_height;
  bool enable_shadow;
  bool enable_pcf_sample_accelerate;
  bool enable_pcss_sample_accelerate;
  Eigen::Matrix<float, 4, 4> mvp;
  Eigen::Matrix<float, 4, 4> mv;
  std::vector<float> z_buffer;
  int get_index(int x, int y);
  void look_at(const Scene &) override;
  bool in_shadow(const Eigen::Vector3f &point_pos,
                 const Eigen::Vector3f &normal) override;
  float in_shadow_pcf(const Eigen::Vector3f &point_pos,
                      const Eigen::Vector3f &normal) override;
  float in_shadow_pcss(const Eigen::Vector3f &point_pos,
                       const Eigen::Vector3f &normal) override;
  void generate_penumbra_mask_block(const Scene &scene,
                                    std::vector<SHADOW_STATUS> &penumbra_mask,
                                    std::vector<float> &penumbra_mask_blur,
                                    int start_row, int start_col, int block_row,
                                    int block_col) override;
};