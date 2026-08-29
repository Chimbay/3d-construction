#pragma once

#include "components/orbit_camera.h"
#include "glm/ext/vector_float4.hpp"
#include "vk_descriptors.h"
#include "vulkan/vulkan_core.h"
#include <functional>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>
#include <vk_types.h>
#include <deletion_queue.h>

struct ComputePushConstants {
  glm::vec4 data1;
  glm::vec4 data2;
  glm::vec4 data3;
  glm::vec4 data4;
};

struct SplatPushConstants {
  glm::mat4 viewProj;
  int       pointCount;
  int       pointSize;
  int       imageWidth;
  int       imageHeight;
};

struct ComputeEffect {
  const char *name;

  VkPipeline pipeline;
  VkPipelineLayout layout;

  ComputePushConstants data;
};

struct FrameData {
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;

  VkSemaphore _swapchainSemaphore, _renderSemaphore;
  VkFence _renderFence;
  DeletionQueue _deletionQueue;
};
constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
  DescriptorAllocator globalDescriptorAllocator;
  VkDescriptorSet _drawImageDescriptors;
  VkDescriptorSetLayout _drawImageDescriptorLayout;

  FrameData _frames[FRAME_OVERLAP];
  VkQueue _graphicsQueue;
  uint32_t _graphicsQueueFamily;
  VmaAllocator _allocator;
  Allocatedimage _drawImage;
  VkExtent2D _drawExtent;

  bool _isInitialized{false};
  int _frameNumber{0};
  bool stop_rendering{false};
  VkExtent2D _windowExtent{1280, 800};
  struct SDL_Window *_window{nullptr};
  DeletionQueue _mainDeletionQueue;

  static VulkanEngine &Get();

  // Vulkan library handle
  VkInstance _instance;
  // Vulkan debug output handle
  VkDebugUtilsMessengerEXT _debug_messenger;
  // GPU chosen as the default device
  VkPhysicalDevice _chosenGPU;
  // Vulkan device for commands
  VkDevice _device;
  // Vulkan window surface
  VkSurfaceKHR _surface;

  // Compute effect
  std::vector<ComputeEffect> backgroundEffects;
  int currentBackgroundEffect{0};

  // initializes everything in the engine
  void init();

  // shuts down the engine
  void cleanup();

  // draw loop
  void draw();

  void draw_background(VkCommandBuffer);

  // run main loop
  void run();

  // Setting up swapchain
  VkSwapchainKHR _swapchain;
  VkFormat _swapchainImageFormat;

  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;
  VkExtent2D _swapchainExtent;

  // Frame structure
  FrameData &get_current_frame() {
    return _frames[_frameNumber % FRAME_OVERLAP];
  }

  VkPipeline _gradientPipeline;
  VkPipelineLayout _gradientPipelineLayout;

  // immediate submit structures
  VkFence _immFence;
  VkCommandBuffer _immCommandBuffer;
  VkCommandPool _immCommandPool;

  void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);

  void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

  // Gaussian splat viewer state
  OrbitCamera           _camera;
  AllocatedBuffer       _gaussianBuffer{};
  uint32_t              _gaussianCount = 0;
  VkPipeline            _splatPipeline = VK_NULL_HANDLE;
  VkPipelineLayout      _splatPipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSet       _splatDescriptors = VK_NULL_HANDLE;
  VkDescriptorSetLayout _splatDescriptorLayout = VK_NULL_HANDLE;
  int                   _splatPointSize = 1;
  std::string           _plyPathInput;  // ImGui InputText buffer
  bool                  _mouseDragging = false;

  void load_ply(const std::string &path);
  void draw_splats(VkCommandBuffer cmd);

private:
  void init_vulkan();
  void init_descriptors();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();

  void init_pipelines();
  void init_background_pipelines();
  void init_splat_pipeline();

  void init_imgui();

  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();

  AllocatedBuffer create_buffer(size_t size, VkBufferUsageFlags usage);
  void            destroy_buffer(AllocatedBuffer &buf);
};
