#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "display.h"
#include "primitives/cube/cube.h"
#include "primitives/grid/grid.h"
#include "scene.h"

#define VIDEO_WIDTH 800
#define VIDEO_HEIGHT 600
#define WINDOW_NAME "Program"

bool windowOk(GLFWwindow *window) {
  if (!window) {
    std::cerr << "Failed to create a window";
    glfwTerminate();
    return false;
  }
  return true;
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
  }

  // Windows: OpenGL 4.3 Core (AMD RX 7900 XTX)
  /*
   * glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   * glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   * glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   */

  // macOS: OpenGL 4.1 Core (M2 — Apple's maximum supported version)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required for Core Profile on macOS

  GLFWwindow *window = glfwCreateWindow(VIDEO_WIDTH, VIDEO_HEIGHT, WINDOW_NAME, nullptr, nullptr);

  if (!windowOk(window))
    return -1;

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return -1;
  }

  // Create the display
  Scene scene;
  scene.add(std::make_unique<Cube>());
  scene.add(std::make_unique<Grid>());
  Display display(window, scene);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
