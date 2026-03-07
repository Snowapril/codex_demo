#pragma once

#import <Metal/Metal.h>

#include <cstddef>
#include <memory>
#include <vector>

#include "metal_command_queue.h"
#include "reng/backend.h"
#include "reng/device.h"
#include "metal_command_buffer.h"

namespace reng {

class MetalDevice : public BackendDevice {
 public:
  explicit MetalDevice(const DeviceDesc& desc = DeviceDesc());
  void shutdown() override;
  id<MTLDevice> device() const { return _device; }
  CommandQueue* graphicsQueue() const override {
    return _graphicsQueue.get();
  }
  CommandQueue* computeQueue() const override { return _computeQueue.get(); }
  size_t copyQueueCount() const override { return _copyQueues.size(); }
  CommandQueue* copyQueue(size_t index) const override {
    return index < _copyQueues.size() ? _copyQueues[index].get() : nullptr;
  }

 private:
  DeviceDesc _desc;
  id<MTLDevice> _device = nil;
  std::unique_ptr<MetalCommandQueue> _graphicsQueue;
  std::unique_ptr<MetalCommandQueue> _computeQueue;
  std::vector<std::unique_ptr<MetalCommandQueue>> _copyQueues;
};

}  // namespace reng
