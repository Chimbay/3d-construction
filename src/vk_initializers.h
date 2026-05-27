#pragma once

#include "vulkan/vulkan_core.h"
#include <vk_types.h>

namespace vkinit {
VkCommandPoolCreateInfo
    command_pool_create_info(uint32_t, VkCommandPoolCreateFlags);
VkCommandBufferAllocateInfo
    command_buffer_allocate_info(VkCommandPool, uint32_t);
VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);
VkCommandBufferBeginInfo
command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags);
VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2, VkSemaphore);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer);
VkSubmitInfo2
submit_info(VkCommandBufferSubmitInfo *, VkSemaphoreSubmitInfo *, VkSemaphoreSubmitInfo *);
VkImageCreateInfo image_create_info(
    VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent
);
VkImageViewCreateInfo imageview_create_info(
    VkFormat format, VkImage image, VkImageAspectFlags aspectFlags
);
VkRenderingAttachmentInfo attachment_info(
    VkImageView view, VkClearValue *clear,
    VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
);
VkRenderingInfo rendering_info(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);
} // namespace vkinit
