#ifndef CUBE_H
#define CUBE_H

#include "glm/fwd.hpp"

#include "primitives/primitive.h"

class Cube : public Primitive {
private:
  void setupVertices();

public:
  Cube();
  void draw(glm::mat4, glm::mat4, glm::vec3) override;
};

#endif
