#pragma once

#include "reng/command_buffer_pool.h"
#include "vulkan_command_buffer.h"
#include "vulkan_command_queue.h"

namespace reng {

class VulkanCommandBufferPool : public CommandBufferPool {
 public:
  VulkanCommandBufferPool(CommandQueue& queue, VulkanDevice& device)
      : CommandBufferPool(queue), _device(device) {}

  std::unique_ptr<CommandBuffer> allocate() override {
    auto* vulkanQueue = dynamic_cast<VulkanCommandQueue*>(&this->queue());
    if (!vulkanQueue) {
      return nullptr;
    }
    auto buffer = std::make_unique<VulkanCommandBuffer>(_device, *vulkanQueue,
                                                        this->queue().queueType());
    buffer->setTimelineValue(this->queue().timeline()->allocateNext().value);
    return buffer;
  }

 private:
  VulkanDevice& _device;
};

}  // namespace reng
