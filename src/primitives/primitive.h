#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "glad/glad.h"
#include "utils/utils.h"
#include <glm/mat4x4.hpp>

class Primitive {
protected:
  GLuint vao[1];
  GLuint vbo[1];
  GLuint shaderProgram;
  GLuint mvLoc, pLoc;

  void initShader(const char *vert, const char *frag) {
    shaderProgram = Utils::createShaderProgram(vert, frag);
    mvLoc = glGetUniformLocation(shaderProgram, "mv_matrix");
    pLoc = glGetUniformLocation(shaderProgram, "p_matrix");
  }

public:
  virtual void draw(glm::mat4 vMat, glm::mat4 pMat, glm::vec3) = 0;
  virtual ~Primitive() = default;
};

#endif
