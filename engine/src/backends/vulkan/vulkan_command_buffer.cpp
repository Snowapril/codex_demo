#include "vulkan_command_buffer.h"

#include "reng/logger.h"
#include "reng/reng.h"
#include "vulkan_swapchain.h"
#include "vulkan_utils.h"

namespace reng {

namespace {
VkAttachmentLoadOp toVkLoadOp(LoadAction action) {
  switch (action) {
    case LoadAction::Load:
      return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadAction::Clear:
      return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadAction::DontCare:
    default:
      return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }
}

VkAttachmentStoreOp toVkStoreOp(StoreAction action) {
  switch (action) {
    case StoreAction::Store:
      return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreAction::DontCare:
    default:
      return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
}
}  // namespace

VulkanCommandBuffer::VulkanCommandBuffer(BackendDevice& device, CommandQueue& queue)
    : CommandBuffer(device, queue) {
  VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  VulkanCommandQueue& vulkanQueue = static_cast<VulkanCommandQueue&>(queue);
  auto& vulkanDevice = static_cast<VulkanDevice&>(device);
  poolInfo.queueFamilyIndex = vulkanQueue.familyIndex();
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (!vulkan::check(
          vkCreateCommandPool(vulkanDevice.device(), &poolInfo, nullptr,
                              &_commandPool),
          "vkCreateCommandPool failed")) {
    return;
  }

  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = _commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;
  if (!vulkan::check(
          vkAllocateCommandBuffers(vulkanDevice.device(), &allocInfo,
                                   &_commandBuffer),
          "vkAllocateCommandBuffers failed")) {
    vkDestroyCommandPool(vulkanDevice.device(), _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
  }
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
  auto& vulkanDevice = static_cast<VulkanDevice&>(device());
  VkDevice nativeDevice = vulkanDevice.device();
  if (_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(nativeDevice, _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
    _commandBuffer = VK_NULL_HANDLE;
  }
}

void VulkanCommandBuffer::onBeginCommandBuffer() {
  (void)ensureRecording();
}

void VulkanCommandBuffer::onEndCommandBuffer() {
  if (_recording) {
    if (timestampEnabled() && _timestampPool != VK_NULL_HANDLE) {
      vkCmdWriteTimestamp(_commandBuffer,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          _timestampPool, 1);
    }
    vkEndCommandBuffer(_commandBuffer);
    _recording = false;
  }
}

CommandBufferTiming VulkanCommandBuffer::submit() {
  VulkanCommandQueue& vulkanQueue = static_cast<VulkanCommandQueue&>(queue());
  CommandBufferTiming timing{};
  timing.queue = vulkanQueue.queueType();
  timing.timelineValue = timelineValue();
  RENG_ASSERT(!isRecording(),
              "submit requires endCommandBuffer to be called");
  if (_commandBuffer == VK_NULL_HANDLE) {
    return timing;
  }
  VkTimelineSemaphoreSubmitInfo timelineInfo{
      VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
  uint64_t signalValue = timelineValue();
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &signalValue;

  VkSemaphore signalSemaphore = VK_NULL_HANDLE;
  if (auto* timeline = vulkanQueue.vulkanTimeline()) {
    signalSemaphore = timeline->semaphore();
  }

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.pNext = &timelineInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_commandBuffer;
  submitInfo.signalSemaphoreCount = signalSemaphore != VK_NULL_HANDLE ? 1 : 0;
  submitInfo.pSignalSemaphores =
      signalSemaphore != VK_NULL_HANDLE ? &signalSemaphore : nullptr;

  if (!vulkan::check(
          vkQueueSubmit(vulkanQueue.queue(), 1, &submitInfo, VK_NULL_HANDLE),
          "vkQueueSubmit failed")) {
    return timing;
  }
  vkQueueWaitIdle(vulkanQueue.queue());

  _timestampPool = VK_NULL_HANDLE;
  return timing;
}

bool VulkanCommandBuffer::ensureRecording() {
  if (_commandBuffer == VK_NULL_HANDLE) {
    return false;
  }
  if (_recording) {
    return true;
  }
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (!vulkan::check(vkBeginCommandBuffer(_commandBuffer, &beginInfo),
                     "vkBeginCommandBuffer failed")) {
    return false;
  }
  VulkanCommandQueue& vulkanQueue = static_cast<VulkanCommandQueue&>(queue());
  if (timestampEnabled()) {
    _timestampPool = vulkanQueue.acquireTimestampPool(timelineValue());
    if (_timestampPool != VK_NULL_HANDLE) {
      vkCmdResetQueryPool(_commandBuffer, _timestampPool, 0, 2);
      vkCmdWriteTimestamp(_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          _timestampPool, 0);
    }
  }
  _recording = true;
  return true;
}

void VulkanCommandBuffer::transitionSwapchainImage(
    VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
  if (image == VK_NULL_HANDLE || oldLayout == newLayout) {
    return;
  }

  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
      newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }

  vkCmdPipelineBarrier(_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}

void VulkanCommandBuffer::onBeginBlitPass() {
  (void)ensureRecording();
}

void VulkanCommandBuffer::onEndBlitPass() {}

void VulkanCommandBuffer::onCopyTexture(const ResourceId& src,
                                        const ResourceId& dst) {
  (void)src;
  (void)dst;
  RengLogger::logWarning("copyTexture not implemented for Vulkan backend");
}

void VulkanCommandBuffer::onUploadBuffer(const ResourceId& buffer,
                                         size_t bytes) {
  (void)buffer;
  (void)bytes;
  RengLogger::logWarning("uploadBuffer not implemented for Vulkan backend");
}

void VulkanCommandBuffer::onUploadTexture(const ResourceId& texture,
                                          size_t bytes) {
  (void)texture;
  (void)bytes;
  RengLogger::logWarning("uploadTexture not implemented for Vulkan backend");
}

void VulkanCommandBuffer::onBeginRenderPass(
    const FramebufferDesc& framebuffer) {
  if (!ensureRecording()) {
    return;
  }

  VulkanSwapchain* swap = nullptr;
  if (swapchain()) {
    swap = static_cast<VulkanSwapchain*>(swapchain());
  }
  if (!swap) {
    RengLogger::logWarning("Vulkan render pass missing swapchain");
    return;
  }

  std::vector<VkRenderingAttachmentInfo> colorAttachments;
  std::vector<VkClearValue> clearValues;

  for (const auto& attachment : framebuffer.colorAttachments) {
    if (attachment.resource != swap->swapchainResourceId()) {
      RengLogger::logWarning(
          "Vulkan render pass supports only swapchain color attachment");
      continue;
    }

    VkClearValue clear{};
    clear.color.float32[0] = attachment.clearColor.r;
    clear.color.float32[1] = attachment.clearColor.g;
    clear.color.float32[2] = attachment.clearColor.b;
    clear.color.float32[3] = attachment.clearColor.a;
    clearValues.push_back(clear);

    VkImageView view = swap->currentImageView();
    if (view == VK_NULL_HANDLE) {
      RengLogger::logWarning("Vulkan swapchain image view unavailable");
      continue;
    }
    VkRenderingAttachmentInfo colorInfo{
        VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorInfo.imageView = view;
    colorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorInfo.loadOp = toVkLoadOp(attachment.loadAction);
    colorInfo.storeOp = toVkStoreOp(attachment.storeAction);
    colorInfo.clearValue = clear;
    colorAttachments.push_back(colorInfo);
  }

  if (colorAttachments.empty()) {
    RengLogger::logWarning("Vulkan render pass has no attachments to render");
    return;
  }

  VkImage swapImage = swap->currentImage();
  VkImageLayout currentLayout = swap->currentImageLayout();
  transitionSwapchainImage(swapImage, currentLayout,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  swap->setCurrentImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = {swap->width(), swap->height()};
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount =
      static_cast<uint32_t>(colorAttachments.size());
  renderingInfo.pColorAttachments = colorAttachments.data();
  vkCmdBeginRendering(_commandBuffer, &renderingInfo);
  _renderingActive = true;
}

void VulkanCommandBuffer::onEndRenderPass() {
  if (_commandBuffer == VK_NULL_HANDLE) {
    return;
  }
  if (_renderingActive) {
    vkCmdEndRendering(_commandBuffer);
    _renderingActive = false;
  }
  VulkanSwapchain* swap = nullptr;
  if (swapchain()) {
    swap = static_cast<VulkanSwapchain*>(swapchain());
  }
  if (swap) {
    transitionSwapchainImage(swap->currentImage(),
                             swap->currentImageLayout(),
                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    swap->setCurrentImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }
}

void VulkanCommandBuffer::onDraw(uint32_t vertexCount,
                                 uint32_t instanceCount) {
  (void)instanceCount;
  if (vertexCount == 0) {
    return;
  }
  RengLogger::logWarning(
      "Draw skipped: Vulkan pipeline binding not implemented");
}

void VulkanCommandBuffer::onBeginComputePass() { (void)ensureRecording(); }

void VulkanCommandBuffer::onEndComputePass() {}

void VulkanCommandBuffer::onDispatch(uint32_t x, uint32_t y, uint32_t z) {
  (void)x;
  (void)y;
  (void)z;
  RengLogger::logWarning(
      "Dispatch skipped: Vulkan compute pipeline binding not implemented");
}

void VulkanCommandBuffer::onBeginMLPass() {
  RengLogger::logWarning("ML pass not implemented for Vulkan backend");
}

void VulkanCommandBuffer::onEndMLPass() {}

void VulkanCommandBuffer::onDispatchML(uint32_t x, uint32_t y, uint32_t z) {
  (void)x;
  (void)y;
  (void)z;
  RengLogger::logWarning("ML dispatch not implemented for Vulkan backend");
}

}  // namespace reng
