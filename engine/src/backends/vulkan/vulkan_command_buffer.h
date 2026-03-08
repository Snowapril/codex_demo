#pragma once

#include "reng/command_buffer.h"
#include "reng/render_graph.h"
#include "vulkan_command_queue.h"
#include "vulkan_device.h"

namespace reng {

class VulkanCommandBuffer : public CommandBuffer {
 public:
  explicit VulkanCommandBuffer(VulkanDevice& device,
                               VulkanCommandQueue& queue,
                               QueueType queueType);
  ~VulkanCommandBuffer() override;

  void submit() override;

 protected:
  void onBeginCommandBuffer() override;
  void onEndCommandBuffer() override;
  void onBeginBlitPass() override;
  void onEndBlitPass() override;
  void onCopyTexture(const ResourceId& src, const ResourceId& dst) override;
  void onUploadBuffer(const ResourceId& buffer, size_t bytes) override;
  void onUploadTexture(const ResourceId& texture, size_t bytes) override;
  void onBeginRenderPass(const FramebufferDesc& framebuffer) override;
  void onEndRenderPass() override;
  void onDraw(uint32_t vertexCount, uint32_t instanceCount) override;
  void onBeginComputePass() override;
  void onEndComputePass() override;
  void onDispatch(uint32_t x, uint32_t y, uint32_t z) override;
  void onBeginMLPass() override;
  void onEndMLPass() override;
  void onDispatchML(uint32_t x, uint32_t y, uint32_t z) override;

 private:
  bool ensureRecording();
  void destroyRenderTargets();

  VulkanDevice& _device;
  VulkanCommandQueue& _queue;
  QueueType _queueType = QueueType::Graphics;
  VkCommandPool _commandPool = VK_NULL_HANDLE;
  VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
  VkRenderPass _renderPass = VK_NULL_HANDLE;
  VkFramebuffer _framebuffer = VK_NULL_HANDLE;
  bool _recording = false;
};

}  // namespace reng
