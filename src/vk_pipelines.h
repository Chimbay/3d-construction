#pragma once

#include "vk_images.h"
#include "vulkan/vulkan_core.h"
#include <fstream>
#include <ios>
#include <vector>

namespace vkutil {
bool load_shader_module(
    const char *filePath, VkDevice device, VkShaderModule *outShaderModule
);
}; // namespace vkutil
