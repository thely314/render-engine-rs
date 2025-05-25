#include <Texture.hpp>
#include <algorithm>
#include <cstdio>
Texture::Texture(const char *filename, int desire_channels) {
  data = stbi_load(filename, &width, &height, &channels, desire_channels);
  if (data == nullptr) {
    printf("Error: could not open file %s\n", filename);
  }
}
int Texture::get_index(int x, int y) {
  return channels * (width * (height - y - 1) + x);
  // return channels * (width * y + x);
};
Eigen::Vector3f Texture::get_color(float u, float v) {
  u *= width;
  v *= height;
  u = std::clamp(u, 0.0f, width * 1.0f);
  v = std::clamp(v, 0.0f, height * 1.0f);
  int center_x = round(u), center_y = round(v);
  float h_rate = u + 0.5f - center_x, v_rate = v + 0.5f - center_y;
  int idx[4]{
      get_index(std::max(0, center_x - 1), std::min(height - 1, center_y)),
      get_index(std::min(width - 1, center_x), std::min(height - 1, center_y)),
      get_index(std::max(0, center_x - 1), std::max(0, center_y - 1)),
      get_index(std::min(width - 1, center_x), std::max(0, center_y - 1))};
  Eigen::Vector3f color_top{
      (1 - h_rate) * data[idx[0]] + h_rate * data[idx[1]],
      (1 - h_rate) * data[idx[0] + 1] + h_rate * data[idx[1] + 1],
      (1 - h_rate) * data[idx[0] + 2] + h_rate * data[idx[1] + 2]};
  Eigen::Vector3f color_bottom{
      (1 - h_rate) * data[idx[2]] + h_rate * data[idx[3]],
      (1 - h_rate) * data[idx[2] + 1] + h_rate * data[idx[3] + 1],
      (1 - h_rate) * data[idx[2] + 2] + h_rate * data[idx[3] + 2]};
  return (v_rate * color_top + (1 - v_rate) * color_bottom) / 255.0f;
}
Texture::~Texture() { delete[] data; }