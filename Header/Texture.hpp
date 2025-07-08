#pragma once
#include "global.hpp"
#include <stb_image.h>

class Texture {
public:
  Texture(const char *filename, int desire_channels = 0);
  int get_index(int x, int y);
  Eigen::Vector3f get_color(float u, float v);
  ~Texture();

private:
  unsigned char *data;
  int width;
  int height;
  int channels;
};