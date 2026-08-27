#pragma once

#include <glm/vec4.hpp>
#include <string>
#include <vector>

// One Gaussian, simplified to position + base color for the point-cloud preview.
// vec4 (not vec3) so it's 16-byte aligned for GPU std430 storage buffers.
struct GaussianPoint {
    glm::vec4 position;  // .xyz = world position, .w unused
    glm::vec4 color;     // .rgb = base color from SH DC, .a = opacity
};

// Load a 3DGS PLY file. Returns empty vector on failure (and prints the reason).
std::vector<GaussianPoint> load_3dgs_ply(const std::string &path);
