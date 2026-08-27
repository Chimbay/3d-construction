#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Target-centered orbit camera. The camera sits at distance `radius` from `target`,
// orientation defined by yaw/pitch (spherical coords). All input handling lives in
// the engine — this class only owns state and produces matrices.
class OrbitCamera {
public:
    // Live (rendered) state. Input methods do NOT write to these directly —
    // they write to the matching *_to fields and tick() eases the live values
    // toward them each frame.
    glm::vec3 target = glm::vec3(0.0f);
    float radius = 5.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    // Desired state. Input methods update these; tick() smooths into the live ones.
    glm::vec3 target_to = glm::vec3(0.0f);
    float radius_to = 5.0f;
    float yaw_to = 0.0f;
    float pitch_to = 0.0f;

    float fovY = 1.0471975511965976f;  // 60 degrees in radians
    float zNear = 0.01f;
    float zFar = 1000.0f;

    // Higher = snappier, lower = floatier. ~15 feels polished but responsive.
    float smoothingRate = 15.0f;

    // Advance the smoothing by dt seconds. Call once per frame before reading view().
    void tick(float dt);

    // Set live values equal to desired (no easing). Use after teleport / file load.
    void snap();

    // Apply a mouse drag in pixels. Sensitivity is multiplied internally.
    void rotate(float dx_pixels, float dy_pixels);

    // Apply a scroll-wheel delta. Positive = zoom in, negative = zoom out.
    void zoom(float scroll);

    // Pan the target along the camera-local right (x) and up (y) axes.
    // Distance is in world units; callers typically pass dt-scaled values.
    void pan(float right_amount, float up_amount);

    // First-person-style movement of the target through the scene.
    // forward/right ignore pitch (stay horizontal); up is world-space.
    void move(float forward, float right, float up);

    // World-space camera position derived from target/radius/yaw/pitch.
    glm::vec3 position() const;

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    glm::mat4 view_projection(float aspect) const;
};
