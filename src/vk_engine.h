#pragma once

#include "vk_descriptors.h"
#include "vk_initializers.h"
#include "vk_pipelines.h"
#include "vulkan/vulkan_core.h"
#include <functional>
#include <vector>
#include <vk_types.h>
#include <deletion_queue.h>

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
  VkExtent2D _windowExtent{256, 256};
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

private:
  void init_vulkan();
  void init_descriptors();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();

  void init_pipelines();
  void init_background_pipelines();

  void init_imgui();

  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();
};
