#pragma once
#include "global.hpp"
#include <stb_image.h>

struct Texture {
  Texture(const char *filename);
  int get_index(int x, int y);
  Eigen::Vector3f get_color(float u, float v);
  ~Texture();
  unsigned char *data;
  int width;
  int height;
  int channels;
};