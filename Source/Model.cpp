#include "Model.hpp"
#include <iostream>
Model::~Model() {
  for (auto obj : objects) {
    delete obj;
  }
}
void Model::rasterization(const Eigen::Matrix<float, 4, 4> &mvp,
                          const Eigen::Matrix<float, 3, 3> &normal_mvp,
                          Scene *scene) {
  int i = 0;
  for (auto obj : objects) {
    obj->rasterization(mvp, normal_mvp, scene);
    std::cout << ++i << "\n";
  }
}