#include "metal_device.h"

#include "reng/logger.h"

namespace reng {

MetalDevice::MetalDevice(const DeviceDesc& desc) : _desc(desc) {
  _device = MTLCreateSystemDefaultDevice();
  if (!_device) {
    RengLogger::logError("MTLCreateSystemDefaultDevice returned nil");
    return;
  }
  _graphicsQueue = std::make_unique<MetalCommandQueue>();
  if (!_graphicsQueue->init(*this, QueueType::Graphics)) {
    RengLogger::logError("Failed to create Metal graphics queue");
  }

  _computeQueue = std::make_unique<MetalCommandQueue>();
  if (!_computeQueue->init(*this, QueueType::Compute)) {
    RengLogger::logError("Failed to create Metal compute queue");
  }

  uint32_t copyCount = _desc.copyQueueCount > 0 ? _desc.copyQueueCount : 1;
  _copyQueues.reserve(copyCount);
  for (uint32_t i = 0; i < copyCount; ++i) {
    auto queue = std::make_unique<MetalCommandQueue>();
    if (!queue->init(*this, QueueType::Transfer)) {
      RengLogger::logError("Failed to create Metal copy queue");
      break;
    }
    _copyQueues.push_back(std::move(queue));
  }
}

void MetalDevice::shutdown() {
  for (auto& queue : _copyQueues) {
    if (queue) {
      queue->shutdown();
    }
  }
  _copyQueues.clear();
  if (_computeQueue) {
    _computeQueue->shutdown();
  }
  if (_graphicsQueue) {
    _graphicsQueue->shutdown();
  }
  _computeQueue.reset();
  _graphicsQueue.reset();
  _device = nil;
}

std::unique_ptr<CommandBuffer> MetalDevice::createCommandBuffer(
    QueueType queueType) {
  return std::make_unique<MetalCommandBuffer>(*this, queueType);
}

}  // namespace reng
