#include "components/orbit_camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float ROTATE_SENSITIVITY = 0.005f;   // radians per pixel
constexpr float ZOOM_SENSITIVITY = 0.1f;       // fraction of radius per scroll step
constexpr float PITCH_LIMIT = 1.553343f;       // ~89° in radians, prevents gimbal flip
constexpr float MIN_RADIUS = 0.05f;
}

void OrbitCamera::tick(float dt) {
    // Frame-rate-independent exponential smoothing.
    // t = 1 - e^(-dt * rate). At dt=1/60 and rate=15, t ≈ 0.22 per frame.
    float t = 1.0f - std::exp(-dt * smoothingRate);
    target = glm::mix(target, target_to, t);
    radius = glm::mix(radius, radius_to, t);
    yaw    = glm::mix(yaw,    yaw_to,    t);
    pitch  = glm::mix(pitch,  pitch_to,  t);
}

void OrbitCamera::snap() {
    target = target_to;
    radius = radius_to;
    yaw    = yaw_to;
    pitch  = pitch_to;
}

void OrbitCamera::rotate(float dx_pixels, float dy_pixels) {
    // First-person rotation: keep the camera position fixed and slide the
    // target so the look direction changes around it instead.
    float cp_old = std::cos(pitch_to);
    glm::vec3 oldOffset(
        radius_to * cp_old * std::sin(yaw_to),
        radius_to * std::sin(pitch_to),
        radius_to * cp_old * std::cos(yaw_to)
    );
    glm::vec3 cameraPos = target_to + oldOffset;

    yaw_to   -= dx_pixels * ROTATE_SENSITIVITY;
    pitch_to -= dy_pixels * ROTATE_SENSITIVITY;
    pitch_to  = std::clamp(pitch_to, -PITCH_LIMIT, PITCH_LIMIT);

    float cp_new = std::cos(pitch_to);
    glm::vec3 newOffset(
        radius_to * cp_new * std::sin(yaw_to),
        radius_to * std::sin(pitch_to),
        radius_to * cp_new * std::cos(yaw_to)
    );
    // Move target so camera position stays put: cameraPos = target_to + offset
    target_to = cameraPos - newOffset;
}

void OrbitCamera::zoom(float scroll) {
    // Exponential zoom keeps the feel consistent across distances.
    radius_to *= std::exp(-scroll * ZOOM_SENSITIVITY);
    radius_to = std::max(radius_to, MIN_RADIUS);
}

void OrbitCamera::move(float forward, float right, float up) {
    // Use the desired yaw so input feels direct even mid-rotation.
    glm::vec3 fwd(-std::sin(yaw_to), 0.0f, -std::cos(yaw_to));
    glm::vec3 rgt = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));

    target_to += fwd * (forward * radius_to);
    target_to += rgt * (right   * radius_to);
    target_to += glm::vec3(0, 1, 0) * (up * radius_to);
}

void OrbitCamera::pan(float right_amount, float up_amount) {
    float cp = std::cos(pitch_to);
    glm::vec3 forward(
        cp * std::sin(yaw_to),
        std::sin(pitch_to),
        cp * std::cos(yaw_to)
    );
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up    = glm::normalize(glm::cross(right, forward));

    target_to += right * (right_amount * radius_to);
    target_to += up    * (up_amount    * radius_to);
}

glm::vec3 OrbitCamera::position() const {
    float cp = std::cos(pitch);
    glm::vec3 offset(
        radius * cp * std::sin(yaw),
        radius * std::sin(pitch),
        radius * cp * std::cos(yaw)
    );
    return target + offset;
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(position(), target, glm::vec3(0, 1, 0));
}

glm::mat4 OrbitCamera::projection(float aspect) const {
    glm::mat4 p = glm::perspective(fovY, aspect, zNear, zFar);
    // Vulkan's clip Y is flipped relative to OpenGL's.
    p[1][1] *= -1.0f;
    return p;
}

glm::mat4 OrbitCamera::view_projection(float aspect) const {
    return projection(aspect) * view();
}
