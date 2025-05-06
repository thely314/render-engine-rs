#include <Triangle.hpp>

void Triangle::draw(Scene &scene, const Model &model) {
  vertexs[0].transform_pos.x() /= vertexs[0].transform_pos.w();
  vertexs[0].transform_pos.y() /= vertexs[0].transform_pos.w();
  vertexs[0].transform_pos.z() /= vertexs[0].transform_pos.w();

  vertexs[1].transform_pos.x() /= vertexs[1].transform_pos.w();
  vertexs[1].transform_pos.y() /= vertexs[1].transform_pos.w();
  vertexs[1].transform_pos.z() /= vertexs[1].transform_pos.w();

  vertexs[2].transform_pos.x() /= vertexs[2].transform_pos.w();
  vertexs[2].transform_pos.y() /= vertexs[2].transform_pos.w();
  vertexs[2].transform_pos.z() /= vertexs[2].transform_pos.w();

  vertexs[0].transform_pos.x() =
      (vertexs[0].transform_pos.x() + 1) * 0.5f * scene.width;
  vertexs[0].transform_pos.y() =
      (vertexs[0].transform_pos.y() + 1) * 0.5f * scene.height;

  vertexs[1].transform_pos.x() =
      (vertexs[1].transform_pos.x() + 1) * 0.5f * scene.width;
  vertexs[1].transform_pos.y() =
      (vertexs[1].transform_pos.y() + 1) * 0.5f * scene.height;

  vertexs[2].transform_pos.x() =
      (vertexs[2].transform_pos.x() + 1) * 0.5f * scene.width;
  vertexs[2].transform_pos.y() =
      (vertexs[2].transform_pos.y() + 1) * 0.5f * scene.height;
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
  if (box_top >= scene.height) {
    box_top = scene.height - 1;
  }
  if (box_right >= scene.width) {
    box_right = scene.width - 1;
  }
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
      auto [alpha, beta, gamma] = Triangle::cal_bary_coord_2D(
          vertexs[0].transform_pos.head(2), vertexs[1].transform_pos.head(2),
          vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle::inside_triangle(alpha, beta, gamma)) {
        alpha = alpha / -vertexs[0].transform_pos.w();
        beta = beta / -vertexs[1].transform_pos.w();
        gamma = gamma / -vertexs[2].transform_pos.w();
        float w_inter = 1.0f / (alpha + beta + gamma);
        float point_transform_pos_z =
            w_inter * (alpha * vertexs[0].transform_pos.z() +
                       beta * vertexs[1].transform_pos.z() +
                       gamma * vertexs[2].transform_pos.z());
        if (point_transform_pos_z > scene.z_buffer[scene.get_index(x, y)]) {
          scene.z_buffer[scene.get_index(x, y)] = point_transform_pos_z;
          Eigen::Vector3f point_pos =
              w_inter * (alpha * vertexs[0].pos + beta * vertexs[1].pos +
                         gamma * vertexs[2].pos);
          Eigen::Vector3f point_normal =
              (w_inter * (alpha * vertexs[0].normal + beta * vertexs[1].normal +
                          gamma * vertexs[2].normal))
                  .normalized();
          Eigen::Vector2f point_uv =
              w_inter * (alpha * vertexs[0].texture_coords +
                         beta * vertexs[1].texture_coords +
                         gamma * vertexs[2].texture_coords);
          Vertex point{point_pos,
                       point_normal,
                       {0.5f, 0.5f, 0.5f},
                       //  {148 / 255.f, 121.0 / 255.f, 92.0 / 255.f},
                       point_uv};
          scene.frame_buffer[scene.get_index(x, y)] =
              scene.shader(point, scene, model, tangent, binormal);
        }
      }
    }
  }
}
Triangle::Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2)
    : vertexs{v0, v1, v2} {}
void Triangle::rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                             const Eigen::Matrix<float, 3, 3> &normal_mvp,
                             Scene &scene, const Model &model) {
  vertexs[0].transform_pos = mvp * vertexs[0].pos.homogeneous();
  vertexs[1].transform_pos = mvp * vertexs[1].pos.homogeneous();
  vertexs[2].transform_pos = mvp * vertexs[2].pos.homogeneous();
  vertexs[0].tranfrom_normal = normal_mvp * vertexs[0].normal;
  vertexs[1].tranfrom_normal = normal_mvp * vertexs[1].normal;
  vertexs[2].tranfrom_normal = normal_mvp * vertexs[2].normal;
  // 在齐次化之前进行裁剪
  std::vector<Triangle> triangles{*this};
  crop_triangles_zNear(triangles);
  crop_triangles_zFar(triangles);
  crop_triangles_xLeft(triangles);
  crop_triangles_xRight(triangles);
  crop_triangles_yBottom(triangles);
  crop_triangles_yTop(triangles);
  for (auto &&triangle : triangles) {
    triangle.draw(scene, model);
  }
}
void Triangle::generate_shadowmap(const Eigen::Matrix<float, 4, 4> &mvp,
                                  const Eigen::Matrix<float, 3, 3> &normal_mvp,
                                  spot_light &light) {
  vertexs[0].transform_pos = mvp * vertexs[0].pos.homogeneous();
  vertexs[1].transform_pos = mvp * vertexs[1].pos.homogeneous();
  vertexs[2].transform_pos = mvp * vertexs[2].pos.homogeneous();
  vertexs[0].tranfrom_normal = normal_mvp * vertexs[0].normal;
  vertexs[1].tranfrom_normal = normal_mvp * vertexs[1].normal;
  vertexs[2].tranfrom_normal = normal_mvp * vertexs[2].normal;
  // 在齐次化之前进行裁剪
  std::vector<Triangle> triangles{*this};
  crop_triangles_zNear(triangles);
  crop_triangles_zFar(triangles);
  crop_triangles_xLeft(triangles);
  crop_triangles_xRight(triangles);
  crop_triangles_yBottom(triangles);
  crop_triangles_yTop(triangles);
  for (auto &&triangle : triangles) {
    triangle.draw_shadowmap(light);
  }
}
void Triangle::draw_shadowmap(spot_light &light) {
  vertexs[0].transform_pos.x() /= vertexs[0].transform_pos.w();
  vertexs[0].transform_pos.y() /= vertexs[0].transform_pos.w();
  vertexs[0].transform_pos.z() /= vertexs[0].transform_pos.w();

  vertexs[1].transform_pos.x() /= vertexs[1].transform_pos.w();
  vertexs[1].transform_pos.y() /= vertexs[1].transform_pos.w();
  vertexs[1].transform_pos.z() /= vertexs[1].transform_pos.w();

  vertexs[2].transform_pos.x() /= vertexs[2].transform_pos.w();
  vertexs[2].transform_pos.y() /= vertexs[2].transform_pos.w();
  vertexs[2].transform_pos.z() /= vertexs[2].transform_pos.w();

  vertexs[0].transform_pos.x() =
      (vertexs[0].transform_pos.x() + 1) * 0.5f * light.zbuffer_width;
  vertexs[0].transform_pos.y() =
      (vertexs[0].transform_pos.y() + 1) * 0.5f * light.zbuffer_height;

  vertexs[1].transform_pos.x() =
      (vertexs[1].transform_pos.x() + 1) * 0.5f * light.zbuffer_width;
  vertexs[1].transform_pos.y() =
      (vertexs[1].transform_pos.y() + 1) * 0.5f * light.zbuffer_height;

  vertexs[2].transform_pos.x() =
      (vertexs[2].transform_pos.x() + 1) * 0.5f * light.zbuffer_width;
  vertexs[2].transform_pos.y() =
      (vertexs[2].transform_pos.y() + 1) * 0.5f * light.zbuffer_height;
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
  if (box_top >= light.zbuffer_height) {
    box_top = light.zbuffer_height - 1;
  }
  if (box_right >= light.zbuffer_width) {
    box_right = light.zbuffer_width - 1;
  }
  for (int y = box_bottom; y <= box_top; ++y) {
    for (int x = box_left; x <= box_right; ++x) {
      auto [alpha, beta, gamma] = Triangle::cal_bary_coord_2D(
          vertexs[0].transform_pos.head(2), vertexs[1].transform_pos.head(2),
          vertexs[2].transform_pos.head(2), {x + 0.5f, y + 0.5f});
      if (Triangle::inside_triangle(alpha, beta, gamma)) {
        alpha = alpha / -vertexs[0].transform_pos.w();
        beta = beta / -vertexs[1].transform_pos.w();
        gamma = gamma / -vertexs[2].transform_pos.w();
        float w_inter = 1.0f / (alpha + beta + gamma);
        float point_transform_pos_z =
            w_inter * (alpha * vertexs[0].transform_pos.z() +
                       beta * vertexs[1].transform_pos.z() +
                       gamma * vertexs[2].transform_pos.z());
        if (point_transform_pos_z > light.z_buffer[light.get_index(x, y)]) {
          light.z_buffer[light.get_index(x, y)] = point_transform_pos_z;
        }
      }
    }
  }
}
void set_pos(const Eigen::Vector3f &pos) {}
void Triangle::move(const Eigen::Matrix<float, 4, 4> &modeling_matrix) {
  for (auto &&v : vertexs) {
    Eigen::Vector4f new_pos = modeling_matrix * v.pos.homogeneous();
    Eigen::Vector3f new_normal = modeling_matrix.block<3, 3>(0, 0) * v.normal;
    v.pos = new_pos.head<3>();
    v.normal = new_normal;
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
void crop_triangles_zNear(std::vector<Triangle> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].transform_pos.z() <
          triangles[i].vertexs[j].transform_pos.w()) {
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.transform_pos.z() - A.transform_pos.w()) /
            ((A.transform_pos.z() - A.transform_pos.w()) -
             (C.transform_pos.z() - C.transform_pos.w()));
      t_E = (B.transform_pos.z() - B.transform_pos.w()) /
            ((B.transform_pos.z() - B.transform_pos.w()) -
             (C.transform_pos.z() - C.transform_pos.w()));
      Vertex D = (1 - t_D) * A + t_D * C;
      Vertex E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle(B, E, D));
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.transform_pos.z() - A.transform_pos.w()) /
            ((A.transform_pos.z() - A.transform_pos.w()) -
             (B.transform_pos.z() - B.transform_pos.w()));
      t_C = (A.transform_pos.z() - A.transform_pos.w()) /
            ((A.transform_pos.z() - A.transform_pos.w()) -
             (C.transform_pos.z() - C.transform_pos.w()));
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

void crop_triangles_zFar(std::vector<Triangle> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].transform_pos.z() >
          -triangles[i].vertexs[j].transform_pos.w()) {
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.transform_pos.z() + A.transform_pos.w()) /
            ((A.transform_pos.z() + A.transform_pos.w()) -
             (C.transform_pos.z() + C.transform_pos.w()));
      t_E = (B.transform_pos.z() + B.transform_pos.w()) /
            ((B.transform_pos.z() + B.transform_pos.w()) -
             (C.transform_pos.z() + C.transform_pos.w()));
      Vertex D = (1 - t_D) * A + t_D * C;
      Vertex E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle(B, E, D));
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.transform_pos.z() + A.transform_pos.w()) /
            ((A.transform_pos.z() + A.transform_pos.w()) -
             (B.transform_pos.z() + B.transform_pos.w()));
      t_C = (A.transform_pos.z() + A.transform_pos.w()) /
            ((A.transform_pos.z() + A.transform_pos.w()) -
             (C.transform_pos.z() + C.transform_pos.w()));
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

void crop_triangles_xLeft(std::vector<Triangle> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].transform_pos.x() >
          -triangles[i].vertexs[j].transform_pos.w()) {
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.transform_pos.x() + A.transform_pos.w()) /
            ((A.transform_pos.x() + A.transform_pos.w()) -
             (C.transform_pos.x() + C.transform_pos.w()));
      t_E = (B.transform_pos.x() + B.transform_pos.w()) /
            ((B.transform_pos.x() + B.transform_pos.w()) -
             (C.transform_pos.x() + C.transform_pos.w()));
      Vertex D = (1 - t_D) * A + t_D * C;
      Vertex E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle(B, E, D));
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.transform_pos.x() + A.transform_pos.w()) /
            ((A.transform_pos.x() + A.transform_pos.w()) -
             (B.transform_pos.x() + B.transform_pos.w()));
      t_C = (A.transform_pos.x() + A.transform_pos.w()) /
            ((A.transform_pos.x() + A.transform_pos.w()) -
             (C.transform_pos.x() + C.transform_pos.w()));
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

void crop_triangles_xRight(std::vector<Triangle> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].transform_pos.x() <
          triangles[i].vertexs[j].transform_pos.w()) {
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.transform_pos.x() - A.transform_pos.w()) /
            ((A.transform_pos.x() - A.transform_pos.w()) -
             (C.transform_pos.x() - C.transform_pos.w()));
      t_E = (B.transform_pos.x() - B.transform_pos.w()) /
            ((B.transform_pos.x() - B.transform_pos.w()) -
             (C.transform_pos.x() - C.transform_pos.w()));
      Vertex D = (1 - t_D) * A + t_D * C;
      Vertex E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle(B, E, D));
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.transform_pos.x() - A.transform_pos.w()) /
            ((A.transform_pos.x() - A.transform_pos.w()) -
             (B.transform_pos.x() - B.transform_pos.w()));
      t_C = (A.transform_pos.x() - A.transform_pos.w()) /
            ((A.transform_pos.x() - A.transform_pos.w()) -
             (C.transform_pos.x() - C.transform_pos.w()));
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

void crop_triangles_yBottom(std::vector<Triangle> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].transform_pos.y() >
          -triangles[i].vertexs[j].transform_pos.w()) {
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.transform_pos.y() + A.transform_pos.w()) /
            ((A.transform_pos.y() + A.transform_pos.w()) -
             (C.transform_pos.y() + C.transform_pos.w()));
      t_E = (B.transform_pos.y() + B.transform_pos.w()) /
            ((B.transform_pos.y() + B.transform_pos.w()) -
             (C.transform_pos.y() + C.transform_pos.w()));
      Vertex D = (1 - t_D) * A + t_D * C;
      Vertex E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle(B, E, D));
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.transform_pos.y() + A.transform_pos.w()) /
            ((A.transform_pos.y() + A.transform_pos.w()) -
             (B.transform_pos.y() + B.transform_pos.w()));
      t_C = (A.transform_pos.y() + A.transform_pos.w()) /
            ((A.transform_pos.y() + A.transform_pos.w()) -
             (C.transform_pos.y() + C.transform_pos.w()));
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

void crop_triangles_yTop(std::vector<Triangle> &triangles) {
  const int iter_size = triangles.size();
  int remain_size = 0;
  for (int i = 0; i != iter_size; ++i) {
    int vertex_out_of_box_cnt = 0;
    bool vertex_unavailable[3]{};
    for (int j = 0; j != 3; ++j) {
      if (triangles[i].vertexs[j].transform_pos.y() <
          triangles[i].vertexs[j].transform_pos.w()) {
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      const Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_D, t_E;
      t_D = (A.transform_pos.y() - A.transform_pos.w()) /
            ((A.transform_pos.y() - A.transform_pos.w()) -
             (C.transform_pos.y() - C.transform_pos.w()));
      t_E = (B.transform_pos.y() - B.transform_pos.w()) /
            ((B.transform_pos.y() - B.transform_pos.w()) -
             (C.transform_pos.y() - C.transform_pos.w()));
      Vertex D = (1 - t_D) * A + t_D * C;
      Vertex E = (1 - t_E) * B + t_E * C;
      C = D;
      triangles.push_back(Triangle(B, E, D));
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
      const Vertex &A = triangles[i].vertexs[vertex_idxs[0]];
      Vertex &B = triangles[i].vertexs[vertex_idxs[1]];
      Vertex &C = triangles[i].vertexs[vertex_idxs[2]];
      float t_B, t_C;
      t_B = (A.transform_pos.y() - A.transform_pos.w()) /
            ((A.transform_pos.y() - A.transform_pos.w()) -
             (B.transform_pos.y() - B.transform_pos.w()));
      t_C = (A.transform_pos.y() - A.transform_pos.w()) /
            ((A.transform_pos.y() - A.transform_pos.w()) -
             (C.transform_pos.y() - C.transform_pos.w()));
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