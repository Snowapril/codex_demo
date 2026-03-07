#include "metal_command_queue.h"

#include "metal_device.h"
#include "reng/logger.h"

namespace reng {

std::unique_ptr<QueueTimeline> MetalCommandQueue::createTimeline(
    BackendDevice& device) {
  return std::make_unique<MetalQueueTimeline>(device);
}

bool MetalCommandQueue::initInner() {
  id<MTLDevice> mtlDevice = static_cast<MetalDevice&>(device()).device();
  if (!mtlDevice) {
    RengLogger::logError("Missing Metal device for command queue");
    return false;
  }

  _queue = [mtlDevice newCommandQueue];
  if (!_queue) {
    RengLogger::logError("Failed to create Metal command queue");
    return false;
  }

  return true;
}

void MetalCommandQueue::shutdownInner() { _queue = nil; }

}  // namespace reng
