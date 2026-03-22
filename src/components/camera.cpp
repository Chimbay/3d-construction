#include "camera.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"

Camera::Camera() {
  cameraPosition = glm::vec3();
  cameraOrientation.x = -90.0f;
  cameraOrientation.y = 0.0f;
  normalizeCamera();
}
Camera::Camera(glm::vec3 position) {
  cameraPosition = position;
  cameraOrientation.x = -90.0f;
  cameraOrientation.y = 0.0f;
  normalizeCamera();
}
Camera::Camera(glm::vec3 position, float yaw, float pitch) {
  cameraPosition = position;
  cameraOrientation.x = yaw;
  cameraOrientation.y = pitch;
  normalizeCamera();
}

void Camera::normalizeCamera() {
  glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
  float yaw = cameraOrientation.x;
  float pitch = cameraOrientation.y;

  cameraN = glm::normalize(glm::vec3(
      cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
      sin(glm::radians(pitch)),
      cos(glm::radians(pitch)) * sin(glm::radians(yaw))));

  cameraU = glm::normalize(glm::cross(cameraN, worldUp));
  cameraV = glm::cross(cameraU, cameraN);
}

// Keyboard movements
void Camera::moveForward() { cameraPosition += cameraN * cameraMovementSpeed; }
void Camera::moveBackward() { cameraPosition -= cameraN * cameraMovementSpeed; }
void Camera::strafeLeft() { cameraPosition -= cameraU * cameraMovementSpeed; }
void Camera::strafeRight() { cameraPosition += cameraU * cameraMovementSpeed; }

// Mouse look
void Camera::setAnchor(float x, float y) {
  anchorPos = glm::vec2(x, y);
  baseOrientation = cameraOrientation;
}
void Camera::setOrientation(float yaw, float pitch) {
  cameraOrientation.x = yaw * cameraSensitivity;
  cameraOrientation.y = glm::clamp(pitch * cameraSensitivity, -89.0f, 89.0f);
  normalizeCamera();
}

// Getters
glm::mat4 Camera::getViewMatrix() {
  return glm::lookAt(cameraPosition, cameraPosition + cameraN, cameraV);
}
glm::vec3 Camera::getCameraPosition() { return cameraPosition; }
glm::vec2 Camera::getCameraOrientation() { return cameraOrientation; }
glm::vec2 Camera::getAnchorPos() { return anchorPos; }
glm::vec2 Camera::getBaseOrientation() { return baseOrientation; }
