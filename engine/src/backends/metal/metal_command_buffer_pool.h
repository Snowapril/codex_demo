#pragma once

#include "metal_command_buffer.h"
#include "reng/command_buffer_pool.h"

namespace reng {

class MetalCommandBufferPool : public CommandBufferPool {
 public:
  explicit MetalCommandBufferPool(MetalDevice& device) : _device(device) {}

  std::unique_ptr<CommandBuffer> allocate(QueueType queueType) override {
    return std::make_unique<MetalCommandBuffer>(_device, queueType);
  }

 private:
  MetalDevice& _device;
};

}  // namespace reng
