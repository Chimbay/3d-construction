#include "grid.h"
#include <glm/gtc/type_ptr.hpp>

Grid::Grid() {
  initShader("src/shaders/grid/grid.vert", "src/shaders/grid/grid.frag");

  setupVertices();
}

void Grid::setupVertices() {
  glGenVertexArrays(1, vao);
  glBindVertexArray(vao[0]);
}
void Grid::draw(glm::mat4 vMat, glm::mat4 pMat, glm::vec3 location) {
  glUseProgram(shaderProgram);


  glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(vMat));
  glUniformMatrix4fv(pLoc, 1, GL_FALSE, glm::value_ptr(pMat));

  // adjust OpenGL settings and draw model
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}
