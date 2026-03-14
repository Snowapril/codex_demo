#pragma once

#include "metal_command_buffer.h"
#include "metal_command_queue.h"
#include "reng/command_buffer_pool.h"

namespace reng {

class MetalCommandBufferPool : public CommandBufferPool {
 public:
  MetalCommandBufferPool(CommandQueue& queue, MetalDevice& device)
      : CommandBufferPool(queue), _device(device) {}

  std::unique_ptr<CommandBuffer> allocate() override {
    auto* metalQueue = dynamic_cast<MetalCommandQueue*>(&this->queue());
    if (!metalQueue) {
      return nullptr;
    }
    auto buffer = std::make_unique<MetalCommandBuffer>(
        _device, *metalQueue);
    buffer->setTimelineValue(this->queue().timeline()->allocateNext().value);
    return buffer;
  }

 private:
  MetalDevice& _device;
};

}  // namespace reng
