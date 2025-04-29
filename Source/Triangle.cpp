#include "global.hpp"
#include <Triangle.hpp>
#include <iostream>

void crop_triangles_zNear(std::vector<Triangle_Vec4> &triangles, float zNear) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].pos.z() < triangles[i].vertexs[j].pos.w()) {
        ++vertex_out_of_box_cnt;
        vertex_unavailable[j] = true;
      }
    }
    switch (vertex_out_of_box_cnt) {
    case 0: {
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 1: {
      int vertex_idxs[3];
      if (vertex_unavailable[0]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else if (vertex_unavailable[1]) {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      } else {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.pos.z() - zNear) / (A.pos.z() - C.pos.z());
      t_E = (B.pos.z() - zNear) / (B.pos.z() - C.pos.z());
      float alpha_D = (1 - t_D) / A.pos.w(), beta_D = t_D / C.pos.w();
      float alpha_E = (1 - t_E) / B.pos.w(), beta_E = t_E / C.pos.w();
      Vertex_vec4 D = (1.0f / (alpha_D + beta_D)) * (alpha_D * A + beta_D * C);
      Vertex_vec4 E = (1.0f / (alpha_E + beta_E)) * (alpha_E * B + beta_E * C);
      C = D;
      triangles.push_back(Triangle_Vec4(B, E, D));
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 2: {
      int vertex_idxs[3];
      if (!vertex_unavailable[0]) {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      } else if (!vertex_unavailable[1]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.pos.z() - zNear) / (A.pos.z() - B.pos.z());
      t_C = (A.pos.z() - zNear) / (A.pos.z() - C.pos.z());
      float alpha_B = (1 - t_B) / A.pos.w(), beta_B = t_B / B.pos.w();
      float alpha_C = (1 - t_C) / A.pos.w(), beta_C = t_C / C.pos.w();
      B = (1.0f / (alpha_B + beta_B)) * (alpha_B * A + beta_B * B);
      C = (1.0f / (alpha_C + beta_C)) * (alpha_C * A + beta_C * C);
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    }
  }
  std::copy(triangles.begin() + iter_size, triangles.end(),
            triangles.begin() + remain_size);
  triangles.resize(triangles.size() - iter_size + remain_size);
}

void crop_triangles_zFar(std::vector<Triangle_Vec4> &triangles, float zFar) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].pos.z() > -triangles[i].vertexs[j].pos.w()) {
        ++vertex_out_of_box_cnt;
        vertex_unavailable[j] = true;
      }
    }
    switch (vertex_out_of_box_cnt) {
    case 0: {
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 1: {

      int vertex_idxs[3];
      if (vertex_unavailable[0]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else if (vertex_unavailable[1]) {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      } else {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.pos.z() - zFar) / (A.pos.z() - C.pos.z());
      t_E = (B.pos.z() - zFar) / (B.pos.z() - C.pos.z());
      float alpha_D = (1 - t_D) / A.pos.w(), beta_D = t_D / C.pos.w();
      float alpha_E = (1 - t_E) / B.pos.w(), beta_E = t_E / C.pos.w();
      Vertex_vec4 D = (1.0f / (alpha_D + beta_D)) * (alpha_D * A + beta_D * C);
      Vertex_vec4 E = (1.0f / (alpha_E + beta_E)) * (alpha_E * B + beta_E * C);
      C = D;
      triangles.push_back(Triangle_Vec4(B, E, D));
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 2: {
      int vertex_idxs[3];
      if (!vertex_unavailable[0]) {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      } else if (!vertex_unavailable[1]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.pos.z() - zFar) / (A.pos.z() - B.pos.z());
      t_C = (A.pos.z() - zFar) / (A.pos.z() - C.pos.z());
      float alpha_B = (1 - t_B) / A.pos.w(), beta_B = t_B / B.pos.w();
      float alpha_C = (1 - t_C) / A.pos.w(), beta_C = t_C / C.pos.w();
      B = (1.0f / (alpha_B + beta_B)) * (alpha_B * A + beta_B * B);
      C = (1.0f / (alpha_C + beta_C)) * (alpha_C * A + beta_C * C);
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    }
  }
  std::copy(triangles.begin() + iter_size, triangles.end(),
            triangles.begin() + remain_size);
  triangles.resize(triangles.size() - iter_size + remain_size);
}

void crop_triangles_xLeft(std::vector<Triangle_Vec4> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].pos.x() > -triangles[i].vertexs[j].pos.w()) {
        ++vertex_out_of_box_cnt;
        vertex_unavailable[j] = true;
      }
    }
    switch (vertex_out_of_box_cnt) {
    case 0: {
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 1: {

      int vertex_idxs[3];
      if (vertex_unavailable[0]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else if (vertex_unavailable[1]) {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      } else {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.pos.x() + A.pos.w()) /
            ((A.pos.x() + A.pos.w()) - (C.pos.x() + C.pos.w()));
      t_E = (B.pos.x() + B.pos.w()) /
            ((B.pos.x() + B.pos.w()) - (C.pos.x() + C.pos.w()));
      Vertex_vec4 D = (1 - t_D) * A + t_D * C;
      Vertex_vec4 E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle_Vec4(B, E, D));
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 2: {
      int vertex_idxs[3];
      if (!vertex_unavailable[0]) {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      } else if (!vertex_unavailable[1]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.pos.x() + A.pos.w()) /
            ((A.pos.x() + A.pos.w()) - (B.pos.x() + B.pos.w()));
      t_C = (A.pos.x() + A.pos.w()) /
            ((A.pos.x() + A.pos.w()) - (C.pos.x() + C.pos.w()));
      B = (1 - t_B) * A + t_B * B;
      C = (1 - t_C) * A + t_C * C;
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    }
  }
  std::copy(triangles.begin() + iter_size, triangles.end(),
            triangles.begin() + remain_size);
  triangles.resize(triangles.size() - iter_size + remain_size);
}

void crop_triangles_xRight(std::vector<Triangle_Vec4> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].pos.x() < triangles[i].vertexs[j].pos.w()) {
        ++vertex_out_of_box_cnt;
        vertex_unavailable[j] = true;
      }
    }
    switch (vertex_out_of_box_cnt) {
    case 0: {
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 1: {

      int vertex_idxs[3];
      if (vertex_unavailable[0]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else if (vertex_unavailable[1]) {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      } else {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.pos.x() - A.pos.w()) /
            ((A.pos.x() - A.pos.w()) - (C.pos.x() - C.pos.w()));
      t_E = (B.pos.x() - B.pos.w()) /
            ((B.pos.x() - B.pos.w()) - (C.pos.x() - C.pos.w()));
      Vertex_vec4 D = (1 - t_D) * A + t_D * C;
      Vertex_vec4 E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle_Vec4(B, E, D));
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 2: {
      int vertex_idxs[3];
      if (!vertex_unavailable[0]) {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      } else if (!vertex_unavailable[1]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.pos.x() - A.pos.w()) /
            ((A.pos.x() - A.pos.w()) - (B.pos.x() - B.pos.w()));
      t_C = (A.pos.x() - A.pos.w()) /
            ((A.pos.x() - A.pos.w()) - (C.pos.x() - C.pos.w()));
      B = (1 - t_B) * A + t_B * B;
      C = (1 - t_C) * A + t_C * C;
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    }
  }
  std::copy(triangles.begin() + iter_size, triangles.end(),
            triangles.begin() + remain_size);
  triangles.resize(triangles.size() - iter_size + remain_size);
}

void crop_triangles_yBottom(std::vector<Triangle_Vec4> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].pos.y() > -triangles[i].vertexs[j].pos.w()) {
        ++vertex_out_of_box_cnt;
        vertex_unavailable[j] = true;
      }
    }
    switch (vertex_out_of_box_cnt) {
    case 0: {
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 1: {

      int vertex_idxs[3];
      if (vertex_unavailable[0]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else if (vertex_unavailable[1]) {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      } else {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.pos.y() + A.pos.w()) /
            ((A.pos.y() + A.pos.w()) - (C.pos.y() + C.pos.w()));
      t_E = (B.pos.y() + B.pos.w()) /
            ((B.pos.y() + B.pos.w()) - (C.pos.y() + C.pos.w()));
      Vertex_vec4 D = (1 - t_D) * A + t_D * C;
      Vertex_vec4 E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle_Vec4(B, E, D));
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 2: {
      int vertex_idxs[3];
      if (!vertex_unavailable[0]) {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      } else if (!vertex_unavailable[1]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.pos.y() + A.pos.w()) /
            ((A.pos.y() + A.pos.w()) - (B.pos.y() + B.pos.w()));
      t_C = (A.pos.y() + A.pos.w()) /
            ((A.pos.y() + A.pos.w()) - (C.pos.y() + C.pos.w()));
      B = (1 - t_B) * A + t_B * B;
      C = (1 - t_C) * A + t_C * C;
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    }
  }
  std::copy(triangles.begin() + iter_size, triangles.end(),
            triangles.begin() + remain_size);
  triangles.resize(triangles.size() - iter_size + remain_size);
}

void crop_triangles_yTop(std::vector<Triangle_Vec4> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].pos.y() < triangles[i].vertexs[j].pos.w()) {
        ++vertex_out_of_box_cnt;
        vertex_unavailable[j] = true;
      }
    }
    switch (vertex_out_of_box_cnt) {
    case 0: {
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 1: {

      int vertex_idxs[3];
      if (vertex_unavailable[0]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else if (vertex_unavailable[1]) {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      } else {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.pos.y() - A.pos.w()) /
            ((A.pos.y() - A.pos.w()) - (C.pos.y() - C.pos.w()));
      t_E = (B.pos.y() - B.pos.w()) /
            ((B.pos.y() - B.pos.w()) - (C.pos.y() - C.pos.w()));
      Vertex_vec4 D = (1 - t_D) * A + t_D * C;
      Vertex_vec4 E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle_Vec4(B, E, D));
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    case 2: {
      int vertex_idxs[3];
      if (!vertex_unavailable[0]) {
        vertex_idxs[0] = 0;
        vertex_idxs[1] = 1;
        vertex_idxs[2] = 2;
      } else if (!vertex_unavailable[1]) {
        vertex_idxs[0] = 1;
        vertex_idxs[1] = 2;
        vertex_idxs[2] = 0;
      } else {
        vertex_idxs[0] = 2;
        vertex_idxs[1] = 0;
        vertex_idxs[2] = 1;
      }
      const Vertex_vec4 &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_vec4 &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_vec4 &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.pos.y() - A.pos.w()) /
            ((A.pos.y() - A.pos.w()) - (B.pos.y() - B.pos.w()));
      t_C = (A.pos.y() - A.pos.w()) /
            ((A.pos.y() - A.pos.w()) - (C.pos.y() - C.pos.w()));
      B = (1 - t_B) * A + t_B * B;
      C = (1 - t_C) * A + t_C * C;
      if (remain_size != i) {
        triangles[remain_size] = triangles[i];
      }
      ++remain_size;
      break;
    }
    }
  }
  std::copy(triangles.begin() + iter_size, triangles.end(),
            triangles.begin() + remain_size);
  triangles.resize(triangles.size() - iter_size + remain_size);
}
void Triangle_Vec4::rasterization(Scene *scene) {
  vertexs[0].pos.x() /= vertexs[0].pos.w();
  vertexs[0].pos.y() /= vertexs[0].pos.w();
  vertexs[0].pos.z() /= vertexs[0].pos.w();

  vertexs[1].pos.x() /= vertexs[1].pos.w();
  vertexs[1].pos.y() /= vertexs[1].pos.w();
  vertexs[1].pos.z() /= vertexs[1].pos.w();

  vertexs[2].pos.x() /= vertexs[2].pos.w();
  vertexs[2].pos.y() /= vertexs[2].pos.w();
  vertexs[2].pos.z() /= vertexs[2].pos.w();

  vertexs[0].pos.x() = (vertexs[0].pos.x() + 1) * 0.5f * scene->width;
  vertexs[0].pos.y() = (1 - vertexs[0].pos.y()) * 0.5f * scene->height;

  vertexs[1].pos.x() = (vertexs[1].pos.x() + 1) * 0.5f * scene->width;
  vertexs[1].pos.y() = (1 - vertexs[1].pos.y()) * 0.5f * scene->height;

  vertexs[2].pos.x() = (vertexs[2].pos.x() + 1) * 0.5f * scene->width;
  vertexs[2].pos.y() = (1 - vertexs[2].pos.y()) * 0.5f * scene->height;
  // std::cout << vertexs[0].pos << "\n"
  //           << vertexs[1].pos << "\n"
  //           << vertexs[2].pos << "\n";
  int box_left = std::max(
      0, (int)std::min(vertexs[0].pos.x(),
                       std::min(vertexs[1].pos.x(), vertexs[2].pos.x())));
  int box_right =
      std::min(scene->width - 1,
               (int)std::max(vertexs[0].pos.x(),
                             std::max(vertexs[1].pos.x(), vertexs[2].pos.x())));
  int box_bottom = std::max(
      0, (int)std::min(vertexs[0].pos.y(),
                       std::min(vertexs[1].pos.y(), vertexs[2].pos.y())));
  int box_top =
      std::min(scene->height - 1,
               (int)std::max(vertexs[0].pos.y(),
                             std::max(vertexs[1].pos.y(), vertexs[2].pos.y())));
  for (int y = box_bottom; y <= box_top; ++y) {
    for (int x = box_left; x <= box_right; ++x) {
      auto [alpha, beta, gamma] = Triangle::cal_bary_coord_2D(
          vertexs[0].pos.head(2), vertexs[1].pos.head(2),
          vertexs[2].pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle::inside_triangle(alpha, beta, gamma)) {
        alpha = alpha / vertexs[0].pos.w();
        beta = beta / vertexs[1].pos.w();
        gamma = gamma / vertexs[2].pos.w();
        float w_inter = 1.0f / (alpha + beta + gamma);
        Eigen::Vector4f pixel_pos =
            w_inter * (alpha * vertexs[0].pos + beta * vertexs[1].pos +
                       gamma * vertexs[2].pos);
        if (pixel_pos.z() > scene->z_buffer[scene->get_index(x, y)]) {
          scene->z_buffer[scene->get_index(x, y)] = pixel_pos.z();
          scene->frame_buffer[scene->get_index(x, y)] =
              3 * (alpha * vertexs[0].normal + beta * vertexs[1].normal +
                   gamma * vertexs[2].normal);
        }
      }
    }
  }
}
Triangle::Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2)
    : vertexs{v0, v1, v2} {}
void Triangle::rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                             const Eigen::Matrix<float, 3, 3> &normal_mvp,
                             Scene *scene) {
  Eigen::Vector4f v0_vec4 = vertexs[0].pos.homogeneous(),
                  v1_vec4 = vertexs[1].pos.homogeneous(),
                  v2_vec4 = vertexs[2].pos.homogeneous();
  v0_vec4 = mvp * v0_vec4;
  v1_vec4 = mvp * v1_vec4;
  v2_vec4 = mvp * v2_vec4;
  // 在齐次化之前进行裁剪
  std::vector<Triangle_Vec4> triangles_vec4{
      Triangle_Vec4({v0_vec4, (normal_mvp * vertexs[0].normal).normalized(),
                     vertexs[0].color, vertexs[0].texture_coords},
                    {v1_vec4, (normal_mvp * vertexs[1].normal).normalized(),
                     vertexs[1].color, vertexs[1].texture_coords},
                    {v2_vec4, (normal_mvp * vertexs[2].normal).normalized(),
                     vertexs[2].color, vertexs[2].texture_coords})};
  crop_triangles_zNear(triangles_vec4, scene->zNear);
  crop_triangles_zFar(triangles_vec4, scene->zFar);
  crop_triangles_xLeft(triangles_vec4);
  crop_triangles_xRight(triangles_vec4);
  crop_triangles_yBottom(triangles_vec4);
  crop_triangles_yTop(triangles_vec4);
  for (auto &&tri : triangles_vec4) {
    tri.rasterization(scene);
  }
}

bool Triangle::inside_triangle(float alpha, float beta, float gamma) {
  return !(alpha < -EPSILON || beta < -EPSILON || gamma < -EPSILON);
}
std::tuple<float, float, float> Triangle::cal_bary_coord_2D(Eigen::Vector2f v0,
                                                            Eigen::Vector2f v1,
                                                            Eigen::Vector2f v2,
                                                            Eigen::Vector2f p) {
  Eigen::Vector2f AB = v1 - v0, BC = v2 - v1;
  Eigen::Vector2f PA = v0 - p, PB = v1 - p;
  float Sabc = AB.x() * BC.y() - BC.x() * AB.y(),
        Spbc = PB.x() * BC.y() - BC.x() * PB.y(),
        Spab = PA.x() * AB.y() - AB.x() * PA.y();
  float alpha = Spbc / Sabc, gamma = Spab / Sabc;
  return {alpha, 1.0f - alpha - gamma, gamma};
}