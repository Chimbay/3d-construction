#include "ply_loader.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fmt/core.h>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {

// SH degree-0 basis function. color = 0.5 + SH_C0 * f_dc_*
constexpr float SH_C0 = 0.28209479177387814f;

float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
float clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }

}  // namespace

std::vector<GaussianPoint> load_3dgs_ply(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        fmt::print("PLY load failed: cannot open {}\n", path);
        return {};
    }

    // Parse header in ASCII line-by-line.
    std::string line;
    std::getline(file, line);
    if (line != "ply") {
        fmt::print("PLY load failed: not a PLY file (missing magic)\n");
        return {};
    }

    bool is_binary_le = false;
    size_t vertex_count = 0;
    std::vector<std::string> properties;  // names in declared order, all assumed float32

    while (std::getline(file, line)) {
        if (line == "end_header") break;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "format") {
            std::string fmt_kind;
            iss >> fmt_kind;
            is_binary_le = (fmt_kind == "binary_little_endian");
        } else if (keyword == "element") {
            std::string elem_name;
            size_t count;
            iss >> elem_name >> count;
            if (elem_name == "vertex") vertex_count = count;
        } else if (keyword == "property") {
            std::string type, name;
            iss >> type >> name;
            // 3DGS exports are all 'float' (float32); we don't bother with other types.
            if (type != "float") {
                fmt::print("PLY load failed: unsupported property type '{}' (only float32 supported)\n", type);
                return {};
            }
            properties.push_back(name);
        }
    }

    if (!is_binary_le) {
        fmt::print("PLY load failed: only binary_little_endian format is supported\n");
        return {};
    }
    if (vertex_count == 0) {
        fmt::print("PLY load failed: vertex count is 0\n");
        return {};
    }

    // Build a name → index map so the order of properties in the file doesn't matter.
    std::unordered_map<std::string, size_t> idx;
    idx.reserve(properties.size());
    for (size_t i = 0; i < properties.size(); ++i) idx[properties[i]] = i;

    auto require = [&](const std::string &name) -> int {
        auto it = idx.find(name);
        if (it == idx.end()) {
            fmt::print("PLY load failed: required property '{}' not in header\n", name);
            return -1;
        }
        return static_cast<int>(it->second);
    };

    int ix = require("x");
    int iy = require("y");
    int iz = require("z");
    int ir = require("f_dc_0");
    int ig = require("f_dc_1");
    int ib = require("f_dc_2");
    int io = require("opacity");
    if (ix < 0 || iy < 0 || iz < 0 || ir < 0 || ig < 0 || ib < 0 || io < 0) return {};

    // Vertex stride in bytes (every property is float32 = 4 bytes).
    const size_t stride = properties.size() * sizeof(float);

    // Read the whole binary body in one shot.
    std::vector<char> body(vertex_count * stride);
    file.read(body.data(), body.size());
    if (file.gcount() != static_cast<std::streamsize>(body.size())) {
        fmt::print("PLY load failed: body shorter than expected ({} of {} bytes)\n",
                   file.gcount(), body.size());
        return {};
    }

    std::vector<GaussianPoint> points;
    points.reserve(vertex_count);

    const float *base = reinterpret_cast<const float *>(body.data());
    const size_t floats_per_vertex = properties.size();

    for (size_t v = 0; v < vertex_count; ++v) {
        const float *p = base + v * floats_per_vertex;

        GaussianPoint gp;
        // 3DGS PLY uses COLMAP convention (Y-down). Negate Y so the rest of
        // the renderer can stay in standard Y-up convention.
        gp.position = glm::vec4(p[ix], -p[iy], p[iz], 1.0f);

        // Convert SH DC coefficients to RGB. Match the 3DGS rendering convention.
        float r = clamp01(0.5f + SH_C0 * p[ir]);
        float g = clamp01(0.5f + SH_C0 * p[ig]);
        float b = clamp01(0.5f + SH_C0 * p[ib]);
        float a = sigmoid(p[io]);
        gp.color = glm::vec4(r, g, b, a);

        points.push_back(gp);
    }

    fmt::print("PLY loaded: {} Gaussians from {}\n", points.size(), path);
    return points;
}
