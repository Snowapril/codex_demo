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

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device,
                                         VulkanCommandQueue& queue,
                                         QueueType queueType)
    : _device(device), _queue(queue), _queueType(queueType) {
  VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = _queue.familyIndex();
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (!vulkan::check(
          vkCreateCommandPool(device.device(), &poolInfo, nullptr,
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
          vkAllocateCommandBuffers(device.device(), &allocInfo,
                                   &_commandBuffer),
          "vkAllocateCommandBuffers failed")) {
    vkDestroyCommandPool(device.device(), _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
  }
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
  VkDevice device = _device.device();
  destroyRenderTargets();
  if (_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, _commandPool, nullptr);
    _commandPool = VK_NULL_HANDLE;
    _commandBuffer = VK_NULL_HANDLE;
  }
}

void VulkanCommandBuffer::onBeginCommandBuffer() {
  (void)ensureRecording();
}

void VulkanCommandBuffer::onEndCommandBuffer() {
  if (_recording) {
    vkEndCommandBuffer(_commandBuffer);
    _recording = false;
  }
}

void VulkanCommandBuffer::submit() {
  RENG_ASSERT(!isRecording(),
              "submit requires endCommandBuffer to be called");
  if (_commandBuffer == VK_NULL_HANDLE) {
    return;
  }
  VkTimelineSemaphoreSubmitInfo timelineInfo{
      VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
  uint64_t signalValue = timelineValue();
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &signalValue;

  VkSemaphore signalSemaphore = VK_NULL_HANDLE;
  if (auto* timeline = _queue.vulkanTimeline()) {
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
          vkQueueSubmit(_queue.queue(), 1, &submitInfo, VK_NULL_HANDLE),
          "vkQueueSubmit failed")) {
    return;
  }
  vkQueueWaitIdle(_queue.queue());
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
  _recording = true;
  return true;
}

void VulkanCommandBuffer::destroyRenderTargets() {
  VkDevice device = _device.device();
  if (_framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device, _framebuffer, nullptr);
    _framebuffer = VK_NULL_HANDLE;
  }
  if (_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, _renderPass, nullptr);
    _renderPass = VK_NULL_HANDLE;
  }
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

  destroyRenderTargets();

  VulkanSwapchain* swap = nullptr;
  if (swapchain()) {
    swap = static_cast<VulkanSwapchain*>(swapchain());
  }
  if (!swap) {
    RengLogger::logWarning("Vulkan render pass missing swapchain");
    return;
  }

  std::vector<VkAttachmentDescription> attachments;
  std::vector<VkAttachmentReference> references;
  std::vector<VkClearValue> clearValues;
  std::vector<VkImageView> views;

  for (const auto& attachment : framebuffer.colorAttachments) {
    if (attachment.resource != swap->swapchainResourceId()) {
      RengLogger::logWarning(
          "Vulkan render pass supports only swapchain color attachment");
      continue;
    }

    VkAttachmentDescription desc{};
    desc.format = swap->format();
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp = toVkLoadOp(attachment.loadAction);
    desc.storeOp = toVkStoreOp(attachment.storeAction);
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments.push_back(desc);

    VkAttachmentReference ref{};
    ref.attachment = static_cast<uint32_t>(references.size());
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    references.push_back(ref);

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
    views.push_back(view);
  }

  if (attachments.empty()) {
    RengLogger::logWarning("Vulkan render pass has no attachments to render");
    return;
  }

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32_t>(references.size());
  subpass.pColorAttachments = references.data();

  VkRenderPassCreateInfo renderPassInfo{
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (!vulkan::check(
          vkCreateRenderPass(_device.device(), &renderPassInfo, nullptr,
                             &_renderPass),
          "vkCreateRenderPass failed")) {
    return;
  }

  VkFramebufferCreateInfo framebufferInfo{
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  framebufferInfo.renderPass = _renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
  framebufferInfo.pAttachments = views.data();
  framebufferInfo.width = swap->width();
  framebufferInfo.height = swap->height();
  framebufferInfo.layers = 1;

  if (!vulkan::check(
          vkCreateFramebuffer(_device.device(), &framebufferInfo, nullptr,
                              &_framebuffer),
          "vkCreateFramebuffer failed")) {
    return;
  }

  VkRenderPassBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = _renderPass;
  beginInfo.framebuffer = _framebuffer;
  beginInfo.renderArea.offset = {0, 0};
  beginInfo.renderArea.extent = {swap->width(), swap->height()};
  beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  beginInfo.pClearValues = clearValues.data();
  vkCmdBeginRenderPass(_commandBuffer, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::onEndRenderPass() {
  if (_commandBuffer == VK_NULL_HANDLE || _renderPass == VK_NULL_HANDLE) {
    return;
  }
  vkCmdEndRenderPass(_commandBuffer);
  destroyRenderTargets();
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
