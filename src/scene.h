#ifndef SCENE_H
#define SCENE_H

#include "primitives/cube/cube.h"
#include <vector>
class Scene {
public:
  std::vector<std::unique_ptr<Primitive>> objects;

  void add(std::unique_ptr<Primitive> obj) {
    objects.push_back(std::move(obj));
  }

  void draw(glm::mat4 vMat, glm::mat4 pMat) {
    for (auto &obj : objects) {
      obj->draw(vMat, pMat, glm::vec3(0.0f));
    }
  }
};

#endif
