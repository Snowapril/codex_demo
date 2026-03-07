#pragma once

#include "reng/command_buffer_pool.h"
#include "vulkan_command_buffer.h"

namespace reng {

class VulkanCommandBufferPool : public CommandBufferPool {
 public:
  explicit VulkanCommandBufferPool(VulkanDevice& device) : _device(device) {}

  std::unique_ptr<CommandBuffer> allocate(QueueType queueType) override {
    return std::make_unique<VulkanCommandBuffer>(_device, queueType);
  }

 private:
  VulkanDevice& _device;
};

}  // namespace reng
