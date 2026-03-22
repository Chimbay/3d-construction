#ifndef CAMERA_H
#define CAMERA_H

#include "glm/fwd.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

class Camera {
private:
  glm::vec3 cameraPosition;
  glm::vec2 cameraOrientation;
  glm::vec2 anchorPos;
  glm::vec2 baseOrientation;

  const float cameraMovementSpeed = 0.1f;
  const float cameraSensitivity = 1.0f;

  glm::vec3 cameraU = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 cameraV = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cameraN = glm::vec3(0.0f, 0.0f, 1.0f);

  void normalizeCamera();

public:
  Camera();
  Camera(glm::vec3);
  Camera(glm::vec3, float, float);

  // Keyboard movement
  void moveForward();
  void moveBackward();
  void strafeLeft();
  void strafeRight();

  // Mouse look
  /**
   * @brief Records the click position and snapshots the current orientation as the drag base
   * @param x Cursor X at moment of click
   * @param y Cursor Y at moment of click
   */
  void setAnchor(float x, float y);

  /**
   * @brief Sets orientation directly (not additive), with pitch clamping
   * @param yaw Absolute yaw in degrees
   * @param pitch Absolute pitch in degrees
   */
  void setOrientation(float yaw, float pitch);

  // Getters
  glm::mat4 getViewMatrix();
  glm::vec3 getCameraPosition();
  glm::vec2 getAnchorPos();
  glm::vec2 getBaseOrientation();

  /**
   * @brief Returns the cameras orientation
   * @return {yaw, pitch} in degrees
   */
  glm::vec2 getCameraOrientation();
};
#endif
