
#include "Eigen/Core"
#include "assimp/mesh.h"
#include <Eigen/Dense>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#include <iostream>
void processNode(aiNode *node, const aiScene *scene) {}
void processMesh(aiMesh *mesh, const aiScene *scene) {}
Eigen::Matrix<float, 4, 4> get_model_matrix(const Eigen::Vector3f &axis,
                                            float angle,
                                            const Eigen::Vector3f &move) {
  float cos_val = cos(angle), sin_val = sin(angle);
  Eigen::Matrix<float, 3, 3> axis_cross;
  axis_cross << 0.0f, -axis.z(), axis.y(), axis.z(), 0.0f, -axis.x(), -axis.y(),
      axis.x(), 0.0f;
  axis_cross = sin_val * axis_cross +
               cos_val * Eigen::Matrix<float, 3, 3>::Identity() +
               (1 - cos_val) * axis * axis.transpose();
  Eigen::Matrix<float, 4, 4> model_matrix = Eigen::Matrix<float, 4, 4>::Zero();
  model_matrix.block(0, 0, 3, 3) = axis_cross.block(0, 0, 3, 3);
  model_matrix(3, 3) = 1.0f;
  Eigen::Matrix<float, 4, 4> move_matrix =
      Eigen::Matrix<float, 4, 4>::Identity();
  move_matrix(0, 3) = move.x();
  move_matrix(1, 3) = move.y();
  move_matrix(2, 3) = move.z();
  return model_matrix * move_matrix;
};
Eigen::Matrix<float, 4, 4> get_view_matrix(const Eigen::Vector3f &eye_pos,
                                           const Eigen::Vector3f &view_dir) {
  Eigen::Vector3f sky_dir(0, 1, 0);
  sky_dir = (sky_dir - sky_dir.dot(view_dir) * view_dir).normalized();
  Eigen::Vector3f x_dir = view_dir.cross(sky_dir).normalized();
  Eigen::Matrix<float, 4, 4> move_matrix =
      Eigen::Matrix<float, 4, 4>::Identity();
  move_matrix(0, 3) = -eye_pos.x();
  move_matrix(1, 3) = -eye_pos.y();
  move_matrix(2, 3) = -eye_pos.z();
  Eigen::Matrix<float, 3, 3> sub_xyz_matrix;
  sub_xyz_matrix << x_dir, sky_dir, -view_dir;
  Eigen::Matrix<float, 4, 4> xyz_matrix = Eigen::Matrix<float, 4, 4>::Zero();
  xyz_matrix.block(0, 0, 3, 3) = sub_xyz_matrix.block(0, 0, 3, 3);
  xyz_matrix(3, 3) = 1.0f;
  return xyz_matrix * move_matrix;
}
Eigen::Matrix<float, 4, 4> get_projection_matrix(const float fov,
                                                 const float aspect_ratio,
                                                 const float zNear,
                                                 const float zFar) {
  // zNear和zFar需要传入负值，且zNear>zFar
  Eigen::Matrix<float, 4, 4> perspective;
  constexpr float to_radian = M_PI / 360.0f;
  float tan_val = tan(fov * to_radian);
  perspective << -1 / (tan_val * aspect_ratio), 0.0f, 0.0f, 0.0f, 0.0f,
      -1 / tan_val, 0.0f, 0.0f, 0.0f, 0.0f, 2 * (zNear + zFar) / (zNear - zFar),
      -2 * zNear * zFar / (zNear - zFar), 0.0f, 0.0f, 1.0f, 0.0f;
  return perspective;
}
int main() {
  // Assimp::Importer importer;
  // const aiScene *scene =
  //     importer.ReadFile("../models/utah_teapot.obj", aiProcess_Triangulate);
  // if (scene == nullptr) {
  //   std::cout << importer.GetErrorString() << '\n';
  //   return 1;
  // }
}