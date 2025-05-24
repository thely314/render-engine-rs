#include "Triangle.hpp"
#include "Eigen/Core"
#include "Model.hpp"
#include "global.hpp"
#include "light.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
Triangle::Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2)
    : vertexs{v0, v1, v2} {}

void Triangle::modeling(const Eigen::Matrix<float, 4, 4> &modeling_matrix) {
  for (auto &&v : vertexs) {
    Eigen::Vector4f new_pos = modeling_matrix * v.pos.homogeneous();
    Eigen::Vector3f new_normal = modeling_matrix.block<3, 3>(0, 0) * v.normal;
    v.pos = new_pos.head<3>();
    v.normal = new_normal.normalized();
  }
}

void Triangle::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                    const Eigen::Matrix<float, 4, 4> &mv, Model &parent) {
  std::vector<Triangle_rasterization> triangles{*this};
  triangles[0].vertexs[0].transform_pos =
      mvp * triangles[0].vertexs[0].pos.homogeneous();
  triangles[0].vertexs[1].transform_pos =
      mvp * triangles[0].vertexs[1].pos.homogeneous();
  triangles[0].vertexs[2].transform_pos =
      mvp * triangles[0].vertexs[2].pos.homogeneous();
  clip_triangles<2, true>(triangles);
  clip_triangles<2, false>(triangles);
  clip_triangles<0, true>(triangles);
  clip_triangles<0, false>(triangles);
  clip_triangles<1, true>(triangles);
  clip_triangles<1, false>(triangles);
  // TODO: 上锁
  for (auto &&obj : triangles) {
    parent.clip_triangles.push_back(obj);
  }
}

void Triangle_rasterization::modeling(
    const Eigen::Matrix<float, 4, 4> &modeling_matrix) {
  for (auto &&v : vertexs) {
    Eigen::Vector4f new_pos = modeling_matrix * v.pos.homogeneous();
    Eigen::Vector3f new_normal = modeling_matrix.block<3, 3>(0, 0) * v.normal;
    v.pos = new_pos.head<3>();
    v.normal = new_normal.normalized();
  }
}

void Triangle_rasterization::rasterization(
    const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene, const Model &model) {
  rasterization_block(mvp, scene, model, 0, 0, scene.width, scene.height);
}

void Triangle_rasterization::rasterization_shadow_map(
    const Eigen::Matrix<float, 4, 4> &mvp, spot_light &light) {
  rasterization_shadow_map_block(mvp, light, 0, 0, light.zbuffer_width,
                                 light.zbuffer_height);
}

Triangle_rasterization::Triangle_rasterization(
    const Triangle &normal_triangle) {
  vertexs[0] = {normal_triangle.vertexs[0].pos,
                normal_triangle.vertexs[0].normal,
                normal_triangle.vertexs[0].color,
                normal_triangle.vertexs[0].texture_coords,
                {0.0f, 0.0f, 0.0f, 0.0f}};

  vertexs[1] = {normal_triangle.vertexs[1].pos,
                normal_triangle.vertexs[1].normal,
                normal_triangle.vertexs[1].color,
                normal_triangle.vertexs[1].texture_coords,
                {0.0f, 0.0f, 0.0f, 0.0f}};
  vertexs[2] = {normal_triangle.vertexs[2].pos,
                normal_triangle.vertexs[2].normal,
                normal_triangle.vertexs[2].color,
                normal_triangle.vertexs[2].texture_coords,
                {0.0f, 0.0f, 0.0f, 0.0f}};
}
Triangle_rasterization::Triangle_rasterization(const Vertex_rasterization &v0,
                                               const Vertex_rasterization &v1,
                                               const Vertex_rasterization &v2)
    : vertexs{v0, v1, v2} {}

void Triangle_rasterization::rasterization_block(
    const Eigen::Matrix<float, 4, 4> &mvp, Scene &scene, const Model &model,
    int start_row, int start_col, int block_row, int block_col) {
  if (block_row <= 0 || block_col <= 0) {
    return;
  }
  int box_left = (int)std::min(
      vertexs[0].transform_pos.x(),
      std::min(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
  int box_right = std::max(
      vertexs[0].transform_pos.x(),
      std::max(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
  int box_bottom = std::min(
      vertexs[0].transform_pos.y(),
      std::min(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
  int box_top = std::max(
      vertexs[0].transform_pos.y(),
      std::max(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
  box_top = std::clamp(box_top, start_row, start_row + block_row - 1);
  box_bottom = std::clamp(box_bottom, start_row, start_row + block_row - 1);
  box_left = std::clamp(box_left, start_col, start_col + block_col - 1);
  box_right = std::clamp(box_right, start_col, start_col + block_col - 1);
  Eigen::Vector3f edge1 = vertexs[1].pos - vertexs[0].pos,
                  edge2 = vertexs[2].pos - vertexs[0].pos;
  Eigen::Vector2f uv_edge1 =
                      vertexs[1].texture_coords - vertexs[0].texture_coords,
                  uv_edge2 =
                      vertexs[2].texture_coords - vertexs[0].texture_coords;
  Eigen::Vector3f tangent =
      (uv_edge2.y() * edge1 - uv_edge1.y() * edge2).normalized();
  Eigen::Vector3f binormal =
      (uv_edge1.x() * edge2 - uv_edge2.x() * edge1).normalized();
  if (uv_edge2.y() * uv_edge1.x() - uv_edge1.y() * uv_edge2.x() <= 0) {
    tangent = -tangent;
    binormal = -binormal;
  };
  for (int y = box_bottom; y <= box_top; ++y) {
    for (int x = box_left; x <= box_right; ++x) {
      auto [alpha, beta, gamma] = Triangle_rasterization::cal_bary_coord_2D(
          vertexs[0].transform_pos.head(2), vertexs[1].transform_pos.head(2),
          vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle_rasterization::inside_triangle(alpha, beta, gamma)) {
        alpha = alpha / -vertexs[0].transform_pos.w();
        beta = beta / -vertexs[1].transform_pos.w();
        gamma = gamma / -vertexs[2].transform_pos.w();
        float w_inter = 1.0f / (alpha + beta + gamma);
        alpha *= w_inter;
        beta *= w_inter;
        gamma *= w_inter;
        float point_transform_pos_z = alpha * vertexs[0].transform_pos.z() +
                                      beta * vertexs[1].transform_pos.z() +
                                      gamma * vertexs[2].transform_pos.z();
        float point_transform_pos_w = alpha * vertexs[0].transform_pos.w() +
                                      beta * vertexs[1].transform_pos.w() +
                                      gamma * vertexs[2].transform_pos.w();
        point_transform_pos_z /= -point_transform_pos_w;
        point_transform_pos_z = (1.0f + point_transform_pos_z) * 0.5f;
        if (point_transform_pos_z < scene.z_buffer[scene.get_index(x, y)]) {
          scene.z_buffer[scene.get_index(x, y)] = point_transform_pos_z;
          Eigen::Vector3f point_pos = alpha * vertexs[0].pos +
                                      beta * vertexs[1].pos +
                                      gamma * vertexs[2].pos;
          scene.pos_buffer[scene.get_index(x, y)] = alpha * vertexs[0].pos +
                                                    beta * vertexs[1].pos +
                                                    gamma * vertexs[2].pos;
          Eigen::Vector3f point_normal =
              (alpha * vertexs[0].normal + beta * vertexs[1].normal +
               gamma * vertexs[2].normal)
                  .normalized();
          Eigen::Vector2f point_uv = alpha * vertexs[0].texture_coords +
                                     beta * vertexs[1].texture_coords +
                                     gamma * vertexs[2].texture_coords;
          if (model.get_texture(Model::NORMAL_TEXTURE) != nullptr) {
            Eigen::Vector3f TBN_normal =
                (2.0f * model.get_texture(Model::NORMAL_TEXTURE)
                            ->get_color(point_uv.x(), point_uv.y()) -
                 Eigen::Vector3f{1.0f, 1.0f, 1.0f})
                    .normalized();
            Eigen::Vector3f tangent_orthogonal =
                (tangent - tangent.dot(point_normal) * point_normal)
                    .normalized();
            Eigen::Vector3f binormal_orthogonal =
                point_normal.cross(tangent_orthogonal).normalized();
            if (binormal_orthogonal.dot(binormal) <= 0) {
              binormal_orthogonal = -binormal_orthogonal;
            }
            Eigen::Matrix<float, 3, 3> TBN;
            TBN << tangent_orthogonal, binormal_orthogonal, point_normal;
            scene.normal_buffer[scene.get_index(x, y)] =
                (TBN * TBN_normal).normalized();
          } else {
            scene.normal_buffer[scene.get_index(x, y)] = point_normal;
          }
          if (model.get_texture(Model::DIFFUSE_TEXTURE) != nullptr) {
            scene.diffuse_buffer[scene.get_index(x, y)] =
                model.get_texture(Model::DIFFUSE_TEXTURE)
                    ->get_color(point_uv.x(), point_uv.y());
          } else {
            scene.diffuse_buffer[scene.get_index(x, y)] = {0.5f, 0.5f, 0.5f};
          }
          if (model.get_texture(Model::SPECULAR_TEXTURE) != nullptr) {
            scene.specular_buffer[scene.get_index(x, y)] =
                model.get_texture(Model::SPECULAR_TEXTURE)
                    ->get_color(point_uv.x(), point_uv.y());
          } else {
            scene.specular_buffer[scene.get_index(x, y)] = {0.8f, 0.8f, 0.8f};
          }
          if (model.get_texture(Model::GLOW_TEXTURE) != nullptr) {
            scene.glow_buffer[scene.get_index(x, y)] =
                model.get_texture(Model::GLOW_TEXTURE)
                    ->get_color(point_uv.x(), point_uv.y());
          } else {
            scene.glow_buffer[scene.get_index(x, y)] = {0.0f, 0.0f, 0.0f};
          }
        }
      }
    }
  }
}

void Triangle_rasterization::rasterization_shadow_map_block(
    const Eigen::Matrix<float, 4, 4> &mvp, spot_light &light, int start_row,
    int start_col, int block_row, int block_col) {
  if (block_row <= 0 || block_col <= 0) {
    return;
  }
  int box_left = (int)std::min(
      vertexs[0].transform_pos.x(),
      std::min(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
  int box_right = std::max(
      vertexs[0].transform_pos.x(),
      std::max(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
  int box_bottom = std::min(
      vertexs[0].transform_pos.y(),
      std::min(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
  int box_top = std::max(
      vertexs[0].transform_pos.y(),
      std::max(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
  box_top = std::clamp(box_top, start_row, start_row + block_row - 1);
  box_bottom = std::clamp(box_bottom, start_row, start_row + block_row - 1);
  box_left = std::clamp(box_left, start_col, start_col + block_col - 1);
  box_right = std::clamp(box_right, start_col, start_col + block_col - 1);
  for (int y = box_bottom; y <= box_top; ++y) {
    for (int x = box_left; x <= box_right; ++x) {
      auto [alpha, beta, gamma] = Triangle_rasterization::cal_bary_coord_2D(
          vertexs[0].transform_pos.head(2), vertexs[1].transform_pos.head(2),
          vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle_rasterization::inside_triangle(alpha, beta, gamma)) {
        alpha = alpha / -vertexs[0].transform_pos.w();
        beta = beta / -vertexs[1].transform_pos.w();
        gamma = gamma / -vertexs[2].transform_pos.w();
        float w_inter = 1.0f / (alpha + beta + gamma);
        alpha *= w_inter;
        beta *= w_inter;
        gamma *= w_inter;
        float point_transform_pos_w = alpha * vertexs[0].transform_pos.w() +
                                      beta * vertexs[1].transform_pos.w() +
                                      gamma * vertexs[2].transform_pos.w();
        // 为了方便PCSS计算，shadow map中存的是视图空间的深度而不是NDC空间的
        // 而且NDC空间不线性导致bias很难调
        if (point_transform_pos_w > light.z_buffer[light.get_index(x, y)]) {
          light.z_buffer[light.get_index(x, y)] = point_transform_pos_w;
        }
      }
    }
  }
}

void Triangle_rasterization::rasterization_shadow_map_block(
    const Eigen::Matrix<float, 4, 4> &mvp, directional_light &light,
    int start_row, int start_col, int block_row, int block_col) {
  if (block_row <= 0 || block_col <= 0) {
    return;
  }
  int box_left = (int)std::min(
      vertexs[0].transform_pos.x(),
      std::min(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
  int box_right = std::max(
      vertexs[0].transform_pos.x(),
      std::max(vertexs[1].transform_pos.x(), vertexs[2].transform_pos.x()));
  int box_bottom = std::min(
      vertexs[0].transform_pos.y(),
      std::min(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
  int box_top = std::max(
      vertexs[0].transform_pos.y(),
      std::max(vertexs[1].transform_pos.y(), vertexs[2].transform_pos.y()));
  box_top = std::clamp(box_top, start_row, start_row + block_row - 1);
  box_bottom = std::clamp(box_bottom, start_row, start_row + block_row - 1);
  box_left = std::clamp(box_left, start_col, start_col + block_col - 1);
  box_right = std::clamp(box_right, start_col, start_col + block_col - 1);
  auto projection_to_view_depth = [&light](float depth) -> float {
    return 0.5f *
           (depth * (light.zNear - light.zFar) + (light.zNear + light.zFar));
  };
  for (int y = box_bottom; y <= box_top; ++y) {
    for (int x = box_left; x <= box_right; ++x) {
      auto [alpha, beta, gamma] = Triangle_rasterization::cal_bary_coord_2D(
          vertexs[0].transform_pos.head(2), vertexs[1].transform_pos.head(2),
          vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle_rasterization::inside_triangle(alpha, beta, gamma)) {
        float point_transform_pos_w = alpha * vertexs[0].transform_pos.z() +
                                      beta * vertexs[1].transform_pos.z() +
                                      gamma * vertexs[2].transform_pos.z();
        point_transform_pos_w = projection_to_view_depth(point_transform_pos_w);
        // 为了方便PCSS计算，shadow map中存的是视图空间的深度而不是NDC空间的
        // 而且NDC空间不线性导致bias很难调
        if (point_transform_pos_w > light.z_buffer[light.get_index(x, y)]) {
          light.z_buffer[light.get_index(x, y)] = point_transform_pos_w;
        }
      }
    }
  }
}

void Triangle_rasterization::to_NDC(int width, int height) {
  // z不需要齐次化
  for (int i = 0; i != 3; ++i) {
    vertexs[i].transform_pos.x() /= vertexs[i].transform_pos.w();
    vertexs[i].transform_pos.y() /= vertexs[i].transform_pos.w();
    vertexs[i].transform_pos.x() =
        (vertexs[i].transform_pos.x() + 1) * 0.5f * width;
    vertexs[i].transform_pos.y() =
        (vertexs[i].transform_pos.y() + 1) * 0.5f * height;
  }
}
void Triangle_rasterization::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                                  const Eigen::Matrix<float, 4, 4> &mv,
                                  Model &parent) {
  Eigen::Vector3f view_space_pos[3];
  view_space_pos[0] = (mv * vertexs[0].pos.homogeneous()).head<3>();
  view_space_pos[1] = (mv * vertexs[1].pos.homogeneous()).head<3>();
  view_space_pos[2] = (mv * vertexs[2].pos.homogeneous()).head<3>();
  Eigen::Vector3f normal = (view_space_pos[1] - view_space_pos[0])
                               .cross(view_space_pos[2] - view_space_pos[1]);
  if (normal.z() < -EPSILON) {
    return;
  }
  std::vector<Triangle_rasterization> triangles{*this};
  triangles[0].vertexs[0].transform_pos =
      mvp * triangles[0].vertexs[0].pos.homogeneous();
  triangles[0].vertexs[1].transform_pos =
      mvp * triangles[0].vertexs[1].pos.homogeneous();
  triangles[0].vertexs[2].transform_pos =
      mvp * triangles[0].vertexs[2].pos.homogeneous();
  clip_triangles<2, true>(triangles);
  clip_triangles<2, false>(triangles);
  clip_triangles<0, true>(triangles);
  clip_triangles<0, false>(triangles);
  clip_triangles<1, true>(triangles);
  clip_triangles<1, false>(triangles);
  for (auto obj : triangles) {
    parent.clip_triangles.push_back(obj);
  }
}

template <int N, bool isLess>
void clip_triangles(std::vector<Triangle_rasterization> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if constexpr (isLess) {
        if (triangles[i].vertexs[j].transform_pos[N] <
            triangles[i].vertexs[j].transform_pos.w()) {
          ++vertex_out_of_box_cnt;
          vertex_unavailable[j] = true;
        }
      } else {
        if (triangles[i].vertexs[j].transform_pos[N] >
            -triangles[i].vertexs[j].transform_pos.w()) {
          ++vertex_out_of_box_cnt;
          vertex_unavailable[j] = true;
        }
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
      const Vertex_rasterization &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex_rasterization &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_rasterization &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      if constexpr (isLess) {
        t_D = (A.transform_pos[N] - A.transform_pos.w()) /
              ((A.transform_pos[N] - A.transform_pos.w()) -
               (C.transform_pos[N] - C.transform_pos.w()));
        t_E = (B.transform_pos[N] - B.transform_pos.w()) /
              ((B.transform_pos[N] - B.transform_pos.w()) -
               (C.transform_pos[N] - C.transform_pos.w()));
      } else {
        t_D = (A.transform_pos[N] + A.transform_pos.w()) /
              ((A.transform_pos[N] + A.transform_pos.w()) -
               (C.transform_pos[N] + C.transform_pos.w()));
        t_E = (B.transform_pos[N] + B.transform_pos.w()) /
              ((B.transform_pos[N] + B.transform_pos.w()) -
               (C.transform_pos[N] + C.transform_pos.w()));
      }
      Vertex_rasterization D = (1 - t_D) * A + t_D * C;
      Vertex_rasterization E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle_rasterization(B, E, D));
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
      const Vertex_rasterization &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex_rasterization &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex_rasterization &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      if constexpr (isLess) {
        t_B = (A.transform_pos[N] - A.transform_pos.w()) /
              ((A.transform_pos[N] - A.transform_pos.w()) -
               (B.transform_pos[N] - B.transform_pos.w()));
        t_C = (A.transform_pos[N] - A.transform_pos.w()) /
              ((A.transform_pos[N] - A.transform_pos.w()) -
               (C.transform_pos[N] - C.transform_pos.w()));
      } else {
        t_B = (A.transform_pos[N] + A.transform_pos.w()) /
              ((A.transform_pos[N] + A.transform_pos.w()) -
               (B.transform_pos[N] + B.transform_pos.w()));
        t_C = (A.transform_pos[N] + A.transform_pos.w()) /
              ((A.transform_pos[N] + A.transform_pos.w()) -
               (C.transform_pos[N] + C.transform_pos.w()));
      }
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

std::tuple<float, float, float> Triangle_rasterization::cal_bary_coord_2D(
    Eigen::Vector2f v0, Eigen::Vector2f v1, Eigen::Vector2f v2,
    Eigen::Vector2f p) {
  Eigen::Vector2f AB = v1 - v0, BC = v2 - v1;
  Eigen::Vector2f PA = v0 - p, PB = v1 - p;
  float Sabc = AB.x() * BC.y() - BC.x() * AB.y(),
        Spbc = PB.x() * BC.y() - BC.x() * PB.y(),
        Spab = PA.x() * AB.y() - AB.x() * PA.y();
  float alpha = Spbc / Sabc, gamma = Spab / Sabc;
  return {alpha, 1.0f - alpha - gamma, gamma};
}

bool Triangle_rasterization::inside_triangle(float alpha, float beta,
                                             float gamma) {
  return !(alpha < -EPSILON || beta < -EPSILON || gamma < -EPSILON);
}