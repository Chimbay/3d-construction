#include "display.h"
#include "GLFW/glfw3.h"
#include "components/camera.h"
#include "glm/fwd.hpp"
#include "utils/utils.h"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>

// init
Display::Display(GLFWwindow *w, Scene &s) : mainCamera(glm::vec3(0.0f, 0.0f, 8.0f)), scene(s) {
  window = w;

  setPerspectiveMatrix();

  runWindow();
}

void Display::render() {
  // build view matrix, model matrix, and model-view matrix

  vMat = mainCamera.getViewMatrix();
  
  scene.draw(vMat, pMat);
}

// Private fields
void Display::runWindow() {
  while (!glfwWindowShouldClose(window)) {
    keyboardInput();
    mouseInput();

    clearWindow();
    render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}
void Display::clearWindow() {
  glClearColor(0.0, 0.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
}
void Display::setPerspectiveMatrix() {
  int width, height;
  float aspect;
  float radians = 1.0472f;

  glfwGetFramebufferSize(window, &width, &height);
  aspect = (float)width / (float)height;

  pMat = glm::perspective(radians, aspect, 0.1f, 1000.0f);
}

// Public fields
void Display::keyboardInput() {
  auto pressed = [&](int key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
  };

  if (pressed(GLFW_KEY_W))
    mainCamera.moveForward();
  if (pressed(GLFW_KEY_S))
    mainCamera.moveBackward();
  if (pressed(GLFW_KEY_A))
    mainCamera.strafeLeft();
  if (pressed(GLFW_KEY_D))
    mainCamera.strafeRight();
}
void Display::mouseInput() {
  bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;

  if (!pressed) {
    isDragging = false;
    return;
  }

  double mouseX, mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);

  if (!isDragging) {
    mainCamera.setAnchor(mouseX, mouseY);
    isDragging = true;
    return;
  }

  glm::vec2 anchor = mainCamera.getAnchorPos();
  glm::vec2 base = mainCamera.getBaseOrientation();

  float yaw = base.x + (mouseX - anchor.x);
  float pitch = base.y - (mouseY - anchor.y);
  mainCamera.setOrientation(yaw, pitch);
}
