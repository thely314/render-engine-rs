#include "Triangle.hpp"
#include "Eigen/Core"
#include "Model.hpp"
#include "Scene.hpp"
#include "Texture.hpp"
#include "global.hpp"
#include <algorithm>
#include <vector>

Triangle::Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2)
    : vertexs{v0, v1, v2} {}

void Triangle::modeling(const Eigen::Matrix<float, 4, 4> &modeling_matrix) {
  for (auto &&v : vertexs) {
    Eigen::Vector4f new_pos = modeling_matrix * v.pos.homogeneous();
    Eigen::Vector3f new_normal =
        modeling_matrix.inverse().transpose().block<3, 3>(0, 0) * v.normal;
    v.pos = new_pos.head<3>();
    v.normal = new_normal.normalized();
  }
}
template <int N, bool isLess>
void clip_verteies(const std::vector<Vertex_rasterization> &verteies,
                   std::vector<Vertex_rasterization> &output);

void Triangle::clip(const Eigen::Matrix<float, 4, 4> &mvp,
                    const Eigen::Matrix<float, 4, 4> &mv, Model &parent) {
  Eigen::Vector3f view_space_pos[3];
  for (int i = 0; i != 3; ++i) {
    view_space_pos[i] = (mv * vertexs[i].pos.homogeneous()).head<3>();
  }
  Eigen::Vector3f normal = (view_space_pos[1] - view_space_pos[0])
                               .cross(view_space_pos[2] - view_space_pos[1]);
  for (int i = 0; i != 3; ++i) {
    if (normal.dot(view_space_pos[i]) > EPSILON) {
      return;
    }
  }
  std::vector<Vertex_rasterization> result, buffer;
  result.reserve(12);
  buffer.reserve(12);
  for (int i = 0; i != 3; ++i) {
    result.push_back(Vertex_rasterization{
        vertexs[i].pos, vertexs[i].normal, vertexs[i].color,
        vertexs[i].texture_coords, mvp * vertexs[i].pos.homogeneous()});
  }
  clip_verteies<2, true>(result, buffer);
  result.clear();
  clip_verteies<2, false>(buffer, result);
  buffer.clear();
  clip_verteies<1, true>(result, buffer);
  result.clear();
  clip_verteies<1, false>(buffer, result);
  buffer.clear();
  clip_verteies<0, true>(result, buffer);
  result.clear();
  clip_verteies<0, false>(buffer, result);
  for (int i = 0; i < (int)result.size() - 2; ++i) {
    parent.clip_triangles.push_back(
        Triangle_rasterization(result[0], result[i + 1], result[i + 2]));
  }
}

void Triangle_rasterization::modeling(
    const Eigen::Matrix<float, 4, 4> &modeling_matrix) {
  for (auto &&v : vertexs) {
    Eigen::Vector4f new_pos = modeling_matrix * v.pos.homogeneous();
    Eigen::Vector3f new_normal =
        modeling_matrix.inverse().transpose().block<3, 3>(0, 0) * v.normal;
    v.pos = new_pos.head<3>();
    v.normal = new_normal.normalized();
  }
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

void Triangle_rasterization::rasterization_block(Scene &scene,
                                                 const Model &model,
                                                 int start_row, int start_col,
                                                 int block_row, int block_col) {
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
  const auto &diffuse_texture = model.get_texture(Model::DIFFUSE_TEXTURE);
  const auto &specular_texture = model.get_texture(Model::SPECULAR_TEXTURE);
  const auto &glow_texture = model.get_texture(Model::GLOW_TEXTURE);
  const auto &normal_texture = model.get_texture(Model::NORMAL_TEXTURE);
  Eigen::Vector3f tangent, binormal;
  if (normal_texture != nullptr) {
    Eigen::Vector3f edge1 = vertexs[1].pos - vertexs[0].pos,
                    edge2 = vertexs[2].pos - vertexs[0].pos;
    Eigen::Vector2f uv_edge1 =
                        vertexs[1].texture_coords - vertexs[0].texture_coords,
                    uv_edge2 =
                        vertexs[2].texture_coords - vertexs[0].texture_coords;
    if (uv_edge1.x() * uv_edge2.y() - uv_edge2.x() * uv_edge1.y() > 0.0) {
      tangent = (uv_edge2.y() * edge1 - uv_edge1.y() * edge2).normalized();
      binormal = (uv_edge1.x() * edge2 - uv_edge2.x() * edge1).normalized();
    } else {
      tangent = -(uv_edge2.y() * edge1 - uv_edge1.y() * edge2).normalized();
      binormal = -(uv_edge1.x() * edge2 - uv_edge2.x() * edge1).normalized();
    }
  }
  for (int y = box_bottom; y <= box_top; ++y) {
    for (int x = box_left; x <= box_right; ++x) {
      auto [alpha, beta, gamma] = Triangle_rasterization::cal_bary_coord_2D(
          vertexs[0].transform_pos.head(2), vertexs[1].transform_pos.head(2),
          vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle_rasterization::inside_triangle(alpha, beta, gamma)) {
        const int idx = scene.get_index(x, y);
        alpha = alpha / vertexs[0].transform_pos.w();
        beta = beta / vertexs[1].transform_pos.w();
        gamma = gamma / vertexs[2].transform_pos.w();
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
        if (point_transform_pos_z < scene.z_buffer[idx]) {
          scene.z_buffer[idx] = point_transform_pos_z;
          Eigen::Vector3f point_pos = alpha * vertexs[0].pos +
                                      beta * vertexs[1].pos +
                                      gamma * vertexs[2].pos;
          scene.pos_buffer[idx] = alpha * vertexs[0].pos +
                                  beta * vertexs[1].pos +
                                  gamma * vertexs[2].pos;
          Eigen::Vector3f point_normal =
              (alpha * vertexs[0].normal + beta * vertexs[1].normal +
               gamma * vertexs[2].normal)
                  .normalized();
          Eigen::Vector2f point_uv = alpha * vertexs[0].texture_coords +
                                     beta * vertexs[1].texture_coords +
                                     gamma * vertexs[2].texture_coords;

          if (normal_texture != nullptr) {
            Eigen::Vector3f TBN_normal =
                (2.0f * normal_texture->get_color(point_uv.x(), point_uv.y()) -
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
            scene.normal_buffer[idx] = (TBN * TBN_normal).normalized();
          } else {
            scene.normal_buffer[idx] = point_normal;
          }
          if (diffuse_texture != nullptr) {
            scene.diffuse_buffer[idx] =
                diffuse_texture->get_color(point_uv.x(), point_uv.y());
          } else {
            scene.diffuse_buffer[idx] = alpha * vertexs[0].color +
                                        beta * vertexs[1].color +
                                        gamma * vertexs[2].color;
          }
          if (specular_texture != nullptr) {
            scene.specular_buffer[idx] =
                specular_texture->get_color(point_uv.x(), point_uv.y());
          } else {
            scene.specular_buffer[idx] = {0.8f, 0.8f, 0.8f};
          }
          if (glow_texture != nullptr) {
            scene.glow_buffer[idx] =
                glow_texture->get_color(point_uv.x(), point_uv.y());
          } else {
            scene.glow_buffer[idx] = {0.0f, 0.0f, 0.0f};
          }
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
template <int N, bool isLess>
void clip_verteies(const std::vector<Vertex_rasterization> &verteies,
                   std::vector<Vertex_rasterization> &output) {
  if (verteies.size() < 3) {
    return;
  }
  const int edge_num = verteies.size();
  std::vector<bool> verteies_unavailable(edge_num);
  for (int i = 0; i < edge_num; ++i) {
    if constexpr (isLess) {
      verteies_unavailable[i] =
          verteies[i].transform_pos[N] < verteies[i].transform_pos[3];
    } else {
      verteies_unavailable[i] =
          verteies[i].transform_pos[N] > -verteies[i].transform_pos[3];
    }
  }
  for (int i = 0; i < edge_num; ++i) {
    int left = i;
    int right = (i + 1) % edge_num;
    const Vertex_rasterization &left_vertex = verteies[left];
    const Vertex_rasterization &right_vertex = verteies[right];
    if (!verteies_unavailable[left] && !verteies_unavailable[right]) {
      output.push_back(left_vertex);
    } else if (!verteies_unavailable[left] || !verteies_unavailable[right]) {
      float t;
      if constexpr (isLess) {
        t = (left_vertex.transform_pos[N] - left_vertex.transform_pos[3]) /
            ((left_vertex.transform_pos[N] - left_vertex.transform_pos[3]) -
             (right_vertex.transform_pos[N] - right_vertex.transform_pos[3]));
      } else {
        t = (left_vertex.transform_pos[N] + left_vertex.transform_pos[3]) /
            ((left_vertex.transform_pos[N] + left_vertex.transform_pos[3]) -
             (right_vertex.transform_pos[N] + right_vertex.transform_pos[3]));
      }
      Vertex_rasterization new_vertex =
          (1 - t) * left_vertex + t * right_vertex;
      if (verteies_unavailable[left]) {
        output.push_back(new_vertex);
      } else {
        output.push_back(left_vertex);
        output.push_back(new_vertex);
      }
    }
  }
}
