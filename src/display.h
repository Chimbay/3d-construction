#ifndef WINDOW_H
#define WINDOW_H

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "GLFW/glfw3.h"
#include "components/camera.h"
#include "scene.h"

class Display {
private:
  GLFWwindow *window;
  Camera mainCamera;
  glm::mat4 pMat, vMat;
  bool isDragging = false;
  Scene &scene;

  void setPerspectiveMatrix();

  void runWindow();
  void clearWindow();
  void render();

public:
  Display(GLFWwindow *, Scene &);
  void keyboardInput();
  void mouseInput();
};

#endif
