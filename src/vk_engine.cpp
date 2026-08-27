#include "vk_descriptors.h"
#include "vk_pipelines.h"
#include "ply_loader.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "SDL_events.h"
#include "SDL_video.h"
#include <vk_engine.h>
#include <vk_images.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include "VkBootstrap.h"
#include "vulkan/vulkan_core.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <thread>

constexpr bool bUseValidationLayers = false;

VulkanEngine *loadedEngine = nullptr;

VulkanEngine &VulkanEngine::Get() { return *loadedEngine; }

void VulkanEngine::init() {

  // only one engine initializaiton is allowed with the application.
  assert(loadedEngine == nullptr);
  loadedEngine = this;

  // We initialize SDL and create a window with it.
  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  _window = SDL_CreateWindow(
      "Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      _windowExtent.width, _windowExtent.height, window_flags
  );

  init_vulkan();
  init_swapchain();
  init_commands();
  init_sync_structures();
  init_descriptors();
  init_pipelines();
  init_splat_pipeline();
  init_imgui();

  _isInitialized = true;
}

void VulkanEngine::cleanup() {
  if (_isInitialized) {
    vkDeviceWaitIdle(_device);
    for (int i = 0; i < FRAME_OVERLAP; i++) {
      vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
      vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
      vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
      vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);

      _frames[i]._deletionQueue.flush();
    }

    _mainDeletionQueue.flush();

    destroy_swapchain();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
    SDL_DestroyWindow(_window);
  }

  // clear engine pointer
  loadedEngine = nullptr;
}

void VulkanEngine::draw() {
  // We wait until the gpu has finished rendering the last frame. Timeout of 1 second
  VK_CHECK(vkWaitForFences(
      _device, 1, &get_current_frame()._renderFence, true, 1000000000
  ));

  get_current_frame()._deletionQueue.flush();

  VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

  uint32_t swapchainImageIndex;

  VK_CHECK(vkAcquireNextImageKHR(
      _device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore,
      nullptr, &swapchainImageIndex
  ));

  // Resetting the command buffer and restarting it.
  // Naming it cmd for shorter writing

  VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

  // Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again
  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  // Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  );

  // Start the command buffer recording
  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  _drawExtent.width = _drawImage.imageExtent.width;
  _drawExtent.height = _drawImage.imageExtent.height;

  // Transition draw image into general layout so we can write into it
  vkutil::transition_image(
      cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL
  );

  draw_background(cmd);

  // Transition draw image and swapchain image into their correct transfer layouts
  vkutil::transition_image(
      cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
  );
  vkutil::transition_image(
      cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  );

  // Copy from draw image into swapchain
  vkutil::copy_image_to_image(
      cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent,
      _swapchainExtent
  );

  // Set swapchain image layout to Attachment Optimal so we can draw imgui
  vkutil::transition_image(
      cmd, _swapchainImages[swapchainImageIndex],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  );

  // Draw imgui into the swapchain image
  draw_imgui(cmd, _swapchainImageViews[swapchainImageIndex]);

  // Set swapchain image layout to Present so we can show it on screen
  vkutil::transition_image(
      cmd, _swapchainImages[swapchainImageIndex],
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  );

  // Finalize the command buffer (no more commands, but it can now be executed)
  VK_CHECK(vkEndCommandBuffer(cmd));

  // Prepare the submission to the queue.
  // We want to wait on the _swapchainSemaphore, as that is signaled when the swapchain is ready
  // We will signal the _renderSemaphore, to signal that rendering has finished

  VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      get_current_frame()._swapchainSemaphore
  );
  VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(
      VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore
  );

  VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

  // Submit command buffer to the queue and execute it.
  // _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit2(
      _graphicsQueue, 1, &submit, get_current_frame()._renderFence
  ));

  // This will put the image we just rendered to into the visible window.
  // We want to wait on the _renderSemaphore for that, as its necessary that drawing commands have finished before the image is displayed to the user
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;

  presentInfo.pSwapchains = &_swapchain;
  presentInfo.swapchainCount = 1;

  presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pImageIndices = &swapchainImageIndex;
  VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

  // Increase the number of frames drawn
  _frameNumber++;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd) {
  // If a PLY is loaded, render it instead of the gradient effects.
  if (_gaussianCount > 0 && _splatPipeline != VK_NULL_HANDLE) {
    draw_splats(cmd);
    return;
  }

  ComputeEffect &effect = backgroundEffects[currentBackgroundEffect];

  // bind the selected background compute pipeline
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1,
      &_drawImageDescriptors, 0, nullptr
  );

  vkCmdPushConstants(
      cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
      sizeof(ComputePushConstants), &effect.data
  );

  // execute the compute pipeline dispatch. 16x16 workgroup size, so divide by it
  vkCmdDispatch(
      cmd, std::ceil(_drawExtent.width / 16.0),
      std::ceil(_drawExtent.height / 16.0), 1
  );
}

void VulkanEngine::run() {
  SDL_Event e;
  bool bQuit = false;

  // main loop
  while (!bQuit) {
    // Handle events in the queue
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) bQuit = true;

      if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) stop_rendering = true;
        if (e.window.event == SDL_WINDOWEVENT_RESTORED)  stop_rendering = false;
      }

      // Drag and drop a .ply onto the window
      if (e.type == SDL_DROPFILE) {
        std::string path = e.drop.file;
        SDL_free(e.drop.file);
        load_ply(path);
        _plyPathInput = path;
      }

      // Camera input — but only when ImGui doesn't want the mouse
      ImGuiIO &io = ImGui::GetIO();
      if (!io.WantCaptureMouse) {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
          _mouseDragging = true;
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
          _mouseDragging = false;
        if (e.type == SDL_MOUSEMOTION && _mouseDragging)
          _camera.rotate(float(e.motion.xrel), float(e.motion.yrel));
        if (e.type == SDL_MOUSEWHEEL)
          _camera.zoom(float(e.wheel.y));
      }

      ImGui_ImplSDL2_ProcessEvent(&e);
    }
    if (stop_rendering) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Splat Viewer")) {
      // Resize the InputText buffer if the path string outgrows it
      if (_plyPathInput.capacity() < 512) _plyPathInput.reserve(512);
      ImGui::InputText("PLY path", _plyPathInput.data(), _plyPathInput.capacity());
      ImGui::SameLine();
      if (ImGui::Button("Load")) {
        // InputText writes through the buffer pointer; sync the string length.
        _plyPathInput = std::string(_plyPathInput.c_str());
        if (!_plyPathInput.empty()) load_ply(_plyPathInput);
      }
      ImGui::TextUnformatted("(or drag a .ply onto the window)");
      ImGui::Separator();
      ImGui::Text("Gaussians loaded: %u", _gaussianCount);
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      ImGui::SliderInt("Point size", &_splatPointSize, 0, 8);
      ImGui::SliderFloat("Camera smoothing", &_camera.smoothingRate, 1.0f, 40.0f);
      if (_gaussianCount > 0) {
        ImGui::Text("Camera target: %.2f %.2f %.2f",
                    _camera.target.x, _camera.target.y, _camera.target.z);
        ImGui::Text("Radius: %.2f", _camera.radius);
      }
    }
    ImGui::End();

    // Background effects window (only useful when no PLY is loaded)
    if (_gaussianCount == 0) {
      if (ImGui::Begin("background")) {
        ComputeEffect &selected = backgroundEffects[currentBackgroundEffect];
        ImGui::Text("Selected effect: %s", selected.name);
        ImGui::SliderInt(
            "Effect index", &currentBackgroundEffect, 0,
            int(backgroundEffects.size()) - 1
        );
        ImGui::InputFloat4("data1", (float *)&selected.data.data1);
        ImGui::InputFloat4("data2", (float *)&selected.data.data2);
        ImGui::InputFloat4("data3", (float *)&selected.data.data3);
        ImGui::InputFloat4("data4", (float *)&selected.data.data4);
      }
      ImGui::End();  // must always pair with Begin, even if Begin returned false
    }

    // WASD navigates through the scene, Q/E moves up/down.
    // Poll per-frame for smooth continuous motion.
    {
      ImGuiIO &io = ImGui::GetIO();
      float dt = io.DeltaTime;
      if (!io.WantCaptureKeyboard) {
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        float speed = 0.5f * dt;
        if (keys[SDL_SCANCODE_W]) _camera.move( speed, 0,      0);
        if (keys[SDL_SCANCODE_S]) _camera.move(-speed, 0,      0);
        if (keys[SDL_SCANCODE_A]) _camera.move( 0,    -speed,  0);
        if (keys[SDL_SCANCODE_D]) _camera.move( 0,     speed,  0);
        if (keys[SDL_SCANCODE_E]) _camera.move( 0,     0,      speed);
        if (keys[SDL_SCANCODE_Q]) _camera.move( 0,     0,     -speed);

        // Arrow keys rotate the camera (equivalent to mouse drag).
        // rotate() takes a pixel delta, so scale a per-second rate by dt.
        float rotRate = 300.0f * dt;
        if (keys[SDL_SCANCODE_RIGHT]) _camera.rotate( rotRate, 0);
        if (keys[SDL_SCANCODE_LEFT])  _camera.rotate(-rotRate, 0);
        if (keys[SDL_SCANCODE_UP])    _camera.rotate(0, -rotRate);
        if (keys[SDL_SCANCODE_DOWN])  _camera.rotate(0,  rotRate);
      }
      // Advance the camera smoothing so rendered state eases toward input
      _camera.tick(dt);
    }

    // make imgui calculate internal draw structures
    ImGui::Render();

    draw();
  }
}

void VulkanEngine::init_vulkan() {
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name("Example Vulkan Applicaiton")
                      .request_validation_layers(bUseValidationLayers)
                      .use_default_debug_messenger()
                      .require_api_version(1, 3, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();

  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;
  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // use vkbootstrap to select a gpu.
  // We want a gpu that can write to the SDL surface and supports vulkan 1.3
  // with the correct features
  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                           .set_required_features_13(features)
                                           .set_required_features_12(features12)
                                           .set_surface(_surface)
                                           .select()
                                           .value();

  // create the final vulkan device
  vkb::DeviceBuilder deviceBuilder{physicalDevice};

  vkb::Device vkbDevice = deviceBuilder.build().value();

  // Get the VkDevice handle used in the rest of a vulkan application
  _device = vkbDevice.device;
  _chosenGPU = physicalDevice.physical_device;

  // use vkbootstrap to get a graphics queue
  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  _graphicsQueueFamily =
      vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  // Initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _mainDeletionQueue.push_function([&]() { vmaDestroyAllocator(_allocator); });
}

void VulkanEngine::init_swapchain() {
  create_swapchain(_windowExtent.width, _windowExtent.height);

  // draw image size will match the window
  VkExtent3D drawImageExtent = {_windowExtent.width, _windowExtent.height, 1};

  // Hardcoding the draw fromat to 32 bit float
  _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  _drawImage.imageExtent = drawImageExtent;

  VkImageUsageFlags drawImageUsages{};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info = vkinit::image_create_info(
      _drawImage.imageFormat, drawImageUsages, drawImageExtent
  );

  // For the draw image, we want to allocate it frm gpu local memory
  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(
      _allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image,
      &_drawImage.allocation, nullptr
  );

  // build a image view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(
      _drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT
  );

  VK_CHECK(
      vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView)
  );

  // Add to deletion queues
  _mainDeletionQueue.push_function([=]() {
    vkDestroyImageView(_device, _drawImage.imageView, nullptr);
    vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
  });
}
void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
  vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

  _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  vkb::Swapchain vkbSwapchain =
      swapchainBuilder
          //.use_default_format_selection()
          .set_desired_format(VkSurfaceFormatKHR{
              .format = _swapchainImageFormat,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          // use vsync present mode
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)

          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build()
          .value();

  _swapchainExtent = vkbSwapchain.extent;
  // store swapchain and its related images
  _swapchain = vkbSwapchain.swapchain;
  _swapchainImages = vkbSwapchain.get_images().value();
  _swapchainImageViews = vkbSwapchain.get_image_views().value();
}
void VulkanEngine::destroy_swapchain() {
  vkDestroySwapchainKHR(_device, _swapchain, nullptr);

  // destroy swapchain resources
  for (int i = 0; i < _swapchainImageViews.size(); i++) {

    vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
  }
}

void VulkanEngine::init_commands() {
  // create a command pool for commands submitted to the graphics queue.
  // we also want the pool to allow for resetting of individual command buffers
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
      _graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
  );

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(
        _device, &commandPoolInfo, nullptr, &_frames[i]._commandPool
    ));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo cmdAllocInfo =
        vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(
        _device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer
    ));
  }

  VK_CHECK(
      vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool)
  );

  // allocate the command buffer for immediate submits
  VkCommandBufferAllocateInfo cmdAllocInfo =
      vkinit::command_buffer_allocate_info(_immCommandPool, 1);

  VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer)
  );

  _mainDeletionQueue.push_function([=]() {
    vkDestroyCommandPool(_device, _immCommandPool, nullptr);
  });
}

void VulkanEngine::init_sync_structures() {
  // create syncronization structures
  // One fence to control when the GPU has finished rendering
  // and 2 semaphores to syncronize rendering with swapchain
  // we want the fence to start signalled so we can wait on it on the first
  // frame

  VkFenceCreateInfo fenceCreateInfo =
      vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateFence(
        _device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence
    ));
    VK_CHECK(vkCreateSemaphore(
        _device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore
    ));
    VK_CHECK(vkCreateSemaphore(
        _device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore
    ));
  }

  VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
  _mainDeletionQueue.push_function([=]() {
    vkDestroyFence(_device, _immFence, nullptr);
  });
}

void VulkanEngine::init_descriptors() {
  // pool sized for both compute background (storage image) and splat (storage image + storage buffer)
  std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

  globalDescriptorAllocator.init_pool(_device, 10, sizes);

  // make the descriptor set layout for our compute draw
  {
    DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _drawImageDescriptorLayout =
        builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
  }

  // allocate a descriptor set for our draw image
  _drawImageDescriptors =
      globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imgInfo.imageView = _drawImage.imageView;

  VkWriteDescriptorSet drawImageWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = _drawImageDescriptors,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .pImageInfo = &imgInfo,
  };

  vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

  // make sure both the descriptor allocator and the new layout get cleaned up properly
  _mainDeletionQueue.push_function([&]() {
    globalDescriptorAllocator.destroy_pool(_device);

    vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
  });
}

void VulkanEngine::init_pipelines() { init_background_pipelines(); }

void VulkanEngine::init_background_pipelines() {
  VkPipelineLayoutCreateInfo computeLayout{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .setLayoutCount = 1,
      .pSetLayouts = &_drawImageDescriptorLayout,
  };

  VkPushConstantRange pushConstant{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .size = sizeof(ComputePushConstants),
      .offset = 0,
  };

  computeLayout.pPushConstantRanges = &pushConstant;
  computeLayout.pushConstantRangeCount = 1;

  VK_CHECK(vkCreatePipelineLayout(
      _device, &computeLayout, nullptr, &_gradientPipelineLayout
  ));

  VkShaderModule gradientShader;
  if (!vkutil::load_shader_module(
          "src/shaders/gradient_color.comp.spv", _device, &gradientShader
      )) {
    fmt::print("Error when building the compute shader \n");
  }

  VkShaderModule skyShader;
  if (!vkutil::load_shader_module(
          "src/shaders/sky.comp.spv", _device, &skyShader
      )) {
    fmt::print("Error when building the compute shader \n");
  }

  VkPipelineShaderStageCreateInfo stageinfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = gradientShader,
      .pName = "main",
  };

  VkComputePipelineCreateInfo computePipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .stage = stageinfo,
      .layout = _gradientPipelineLayout,
  };

  ComputeEffect gradient = {
      .name = "gradient", .layout = _gradientPipelineLayout, .data = {}};

  // default colors
  gradient.data.data1 = glm::vec4(1, 0, 0, 1);
  gradient.data.data2 = glm::vec4(0, 0, 1, 1);

  VK_CHECK(vkCreateComputePipelines(
      _device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
      &gradient.pipeline
  ));

  // change the shader module only to create the sky shader
  computePipelineCreateInfo.stage.module = skyShader;

  ComputeEffect sky = {
      .name = "sky", .layout = _gradientPipelineLayout, .data = {}};
  sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

  VK_CHECK(vkCreateComputePipelines(
      _device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
      &sky.pipeline
  ));

  // add the 2 background effects into the array
  backgroundEffects.push_back(gradient);
  backgroundEffects.push_back(sky);

  vkDestroyShaderModule(_device, gradientShader, nullptr);
  vkDestroyShaderModule(_device, skyShader, nullptr);
  _mainDeletionQueue.push_function([=]() {
    vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
    vkDestroyPipeline(_device, sky.pipeline, nullptr);
    vkDestroyPipeline(_device, gradient.pipeline, nullptr);
  });
}

AllocatedBuffer VulkanEngine::create_buffer(size_t size, VkBufferUsageFlags usage) {
  VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  // Host-visible + GPU-readable. On Apple Silicon this is unified memory; on
  // dedicated GPUs it's slower but fine for an upload-once buffer.
  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;

  AllocatedBuffer buf{};
  VK_CHECK(vmaCreateBuffer(
      _allocator, &bufferInfo, &allocInfo, &buf.buffer, &buf.allocation, &buf.info
  ));
  return buf;
}

void VulkanEngine::destroy_buffer(AllocatedBuffer &buf) {
  if (buf.buffer == VK_NULL_HANDLE) return;
  vmaDestroyBuffer(_allocator, buf.buffer, buf.allocation);
  buf = {};
}

void VulkanEngine::init_splat_pipeline() {
  // Descriptor layout: binding 0 = storage image (draw image), binding 1 = storage buffer (gaussians)
  {
    DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    builder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    _splatDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
  }

  // Allocate the descriptor set. We'll write the storage image now (it doesn't
  // change for the lifetime of the engine) and the storage buffer in load_ply().
  _splatDescriptors =
      globalDescriptorAllocator.allocate(_device, _splatDescriptorLayout);

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imgInfo.imageView = _drawImage.imageView;

  VkWriteDescriptorSet imgWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = _splatDescriptors,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .pImageInfo = &imgInfo,
  };
  vkUpdateDescriptorSets(_device, 1, &imgWrite, 0, nullptr);

  // Pipeline layout with push constants for the camera matrix + metadata
  VkPushConstantRange pushConstant{
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .offset = 0,
      .size = sizeof(SplatPushConstants),
  };

  VkPipelineLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .setLayoutCount = 1,
      .pSetLayouts = &_splatDescriptorLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstant,
  };
  VK_CHECK(vkCreatePipelineLayout(
      _device, &layoutInfo, nullptr, &_splatPipelineLayout
  ));

  VkShaderModule splatShader;
  if (!vkutil::load_shader_module(
          "src/shaders/splat.comp.spv", _device, &splatShader
      )) {
    fmt::print("Error building splat compute shader\n");
    return;
  }

  VkPipelineShaderStageCreateInfo stage{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = splatShader,
      .pName = "main",
  };

  VkComputePipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .stage = stage,
      .layout = _splatPipelineLayout,
  };

  VK_CHECK(vkCreateComputePipelines(
      _device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_splatPipeline
  ));

  vkDestroyShaderModule(_device, splatShader, nullptr);

  _mainDeletionQueue.push_function([this]() {
    vkDestroyPipeline(_device, _splatPipeline, nullptr);
    vkDestroyPipelineLayout(_device, _splatPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _splatDescriptorLayout, nullptr);
    if (_gaussianBuffer.buffer != VK_NULL_HANDLE) {
      vmaDestroyBuffer(
          _allocator, _gaussianBuffer.buffer, _gaussianBuffer.allocation
      );
    }
  });
}

void VulkanEngine::load_ply(const std::string &path) {
  auto points = load_3dgs_ply(path);
  if (points.empty()) return;

  // Wait for GPU to finish so we can safely replace the buffer mid-program.
  vkDeviceWaitIdle(_device);

  // Free the previous buffer if any.
  destroy_buffer(_gaussianBuffer);

  // Allocate GPU-visible buffer and memcpy directly into mapped memory.
  size_t bytes = points.size() * sizeof(GaussianPoint);
  _gaussianBuffer = create_buffer(bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  std::memcpy(_gaussianBuffer.info.pMappedData, points.data(), bytes);
  _gaussianCount = static_cast<uint32_t>(points.size());

  // Update descriptor set binding 1 to point at the new buffer.
  VkDescriptorBufferInfo bufInfo{};
  bufInfo.buffer = _gaussianBuffer.buffer;
  bufInfo.offset = 0;
  bufInfo.range = bytes;

  VkWriteDescriptorSet bufWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = _splatDescriptors,
      .dstBinding = 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &bufInfo,
  };
  vkUpdateDescriptorSets(_device, 1, &bufWrite, 0, nullptr);

  // Fit the camera around the model — compute centroid + bounding radius.
  glm::vec3 minP(std::numeric_limits<float>::max());
  glm::vec3 maxP(std::numeric_limits<float>::lowest());
  for (const auto &p : points) {
    minP = glm::min(minP, glm::vec3(p.position));
    maxP = glm::max(maxP, glm::vec3(p.position));
  }
  glm::vec3 center = 0.5f * (minP + maxP);
  float boundsRadius = 0.5f * glm::length(maxP - minP);

  _camera.target_to = center;
  _camera.radius_to = std::max(boundsRadius * 2.0f, 0.5f);
  _camera.yaw_to = 0.0f;
  _camera.pitch_to = 0.0f;
  _camera.snap();  // teleport, don't ease in from the old view
}

void VulkanEngine::draw_splats(VkCommandBuffer cmd) {
  // Clear the draw image first by binding the splat pipeline over a black background.
  // We rely on the fact that draw_background runs after the draw image is transitioned
  // to GENERAL, but the previous frame's data is still there. Clear it explicitly.
  VkClearColorValue clearValue{{0.0f, 0.0f, 0.0f, 1.0f}};
  VkImageSubresourceRange clearRange =
      vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
  vkCmdClearColorImage(
      cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange
  );

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _splatPipeline);
  vkCmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _splatPipelineLayout, 0, 1,
      &_splatDescriptors, 0, nullptr
  );

  float aspect = float(_drawExtent.width) / float(_drawExtent.height);

  SplatPushConstants pc{};
  pc.viewProj = _camera.view_projection(aspect);
  pc.pointCount = static_cast<int>(_gaussianCount);
  pc.pointSize = _splatPointSize;
  pc.imageWidth = static_cast<int>(_drawExtent.width);
  pc.imageHeight = static_cast<int>(_drawExtent.height);

  vkCmdPushConstants(
      cmd, _splatPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
      sizeof(SplatPushConstants), &pc
  );

  // 256 threads per workgroup, one thread per Gaussian.
  uint32_t groupCount = (_gaussianCount + 255) / 256;
  vkCmdDispatch(cmd, groupCount, 1, 1);
}

void VulkanEngine::immediate_submit(
    std::function<void(VkCommandBuffer cmd)> &&function
) {
  VK_CHECK(vkResetFences(_device, 1, &_immFence));
  VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

  VkCommandBuffer cmd = _immCommandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  );

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  function(cmd);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
  VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

  // submit command buffer to the queue and execute it.
  // _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

  VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
}

void VulkanEngine::init_imgui() {
  // 1: create descriptor pool for IMGUI
  // the size of the pool is very oversize
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1000,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .poolSizeCount = (uint32_t)std::size(pool_sizes),
      .pPoolSizes = pool_sizes};

  VkDescriptorPool imguiPool;
  VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

  // 2. Initialize imgui library

  ImGui::CreateContext();

  // this initializes imgui for SDL
  ImGui_ImplSDL2_InitForVulkan(_window);

  // this initializes imgui for VUlkan
  ImGui_ImplVulkan_InitInfo init_info = {
      .Instance = _instance,
      .PhysicalDevice = _chosenGPU,
      .Device = _device,
      .Queue = _graphicsQueue,
      .DescriptorPool = imguiPool,
      .MinImageCount = 3,
      .ImageCount = 3,
      .UseDynamicRendering = true,
  };

  // dynamic rendering parameters for the gui to use
  init_info.PipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &_swapchainImageFormat;

  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&init_info);
  ImGui_ImplVulkan_CreateFontsTexture();

  // add the destroy the imgui created structures
  _mainDeletionQueue.push_function([=]() {
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(_device, imguiPool, nullptr);
  });
}
void VulkanEngine::draw_imgui(
    VkCommandBuffer cmd, VkImageView targetImageView
) {
  VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
      targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  );
  VkRenderingInfo renderInfo =
      vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

  vkCmdBeginRendering(cmd, &renderInfo);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

  vkCmdEndRendering(cmd);
}
