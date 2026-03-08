#include "metal_command_queue.h"

#include "metal_command_buffer_pool.h"
#include "metal_device.h"
#import <Metal/MTL4Counters.h>
#include "reng/logger.h"

namespace reng {

std::unique_ptr<QueueTimeline> MetalCommandQueue::createTimeline(
    BackendDevice& device) {
  return std::make_unique<MetalQueueTimeline>(device);
}

std::unique_ptr<CommandBufferPool> MetalCommandQueue::createCommandBufferPool(
    BackendDevice& device) {
  return std::make_unique<MetalCommandBufferPool>(
      *this, static_cast<MetalDevice&>(device));
}

bool MetalCommandQueue::initInner() {
  id<MTLDevice> mtlDevice = static_cast<MetalDevice&>(device()).device();
  if (!mtlDevice) {
    RengLogger::logError("Missing Metal device for command queue");
    return false;
  }

  _queue = [mtlDevice newMTL4CommandQueue];
  if (!_queue) {
    RengLogger::logError("Failed to create Metal command queue");
    return false;
  }

  return true;
}

id<MTL4CounterHeap> MetalCommandQueue::acquireTimestampHeap(
    uint64_t timelineValue) {
  auto* timeline = metalTimeline();
  uint64_t completed = timeline ? timeline->completedValue() : 0;
  for (auto& entry : _timestampHeaps) {
    if (entry.heap && completed >= entry.lastValue) {
      entry.lastValue = timelineValue;
      return entry.heap;
    }
  }

  id<MTLDevice> mtlDevice = static_cast<MetalDevice&>(device()).device();
  if (!mtlDevice) {
    return nil;
  }
  MTL4CounterHeapDescriptor* desc = [[MTL4CounterHeapDescriptor alloc] init];
  desc.type = MTL4CounterHeapTypeTimestamp;
  desc.count = 2;
  id<MTL4CounterHeap> heap = [mtlDevice newCounterHeapWithDescriptor:desc];
  if (!heap) {
    RengLogger::logWarning("Failed to create Metal timestamp counter heap");
    return nil;
  }
  _timestampHeaps.push_back({heap, timelineValue});
  return heap;
}

void MetalCommandQueue::shutdownInner() {
  _timestampHeaps.clear();
  _queue = nil;
}

}  // namespace reng
