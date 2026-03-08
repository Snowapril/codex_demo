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
    if (entry.heap && !entry.pending && completed >= entry.lastValue) {
      entry.lastValue = timelineValue;
      entry.pending = true;
      _timestampHeapByValue[timelineValue] = &entry - _timestampHeaps.data();
      return entry.heap;
    }
  }

  id<MTLDevice> mtlDevice = static_cast<MetalDevice&>(device()).device();
  if (!mtlDevice) {
    return nil;
  }
  if (![mtlDevice respondsToSelector:@selector(newCounterHeapWithDescriptor:)]) {
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
  _timestampHeaps.push_back({heap, timelineValue, true});
  _timestampHeapByValue[timelineValue] = _timestampHeaps.size() - 1;
  return heap;
}

bool MetalCommandQueue::resolveTimestamp(uint64_t timelineValue,
                                         CommandBufferTiming& timing) {
  auto it = _timestampHeapByValue.find(timelineValue);
  if (it == _timestampHeapByValue.end()) {
    return false;
  }
  if (it->second >= _timestampHeaps.size()) {
    return false;
  }
  auto& entry = _timestampHeaps[it->second];
  if (!entry.heap || !entry.pending) {
    return false;
  }
  auto* timeline = metalTimeline();
  uint64_t completed = timeline ? timeline->completedValue() : 0;
  if (completed < timelineValue) {
    return false;
  }

  NSData* data = [entry.heap resolveCounterRange:NSMakeRange(0, 2)];
  if (!data || data.length < sizeof(MTL4TimestampHeapEntry) * 2) {
    RengLogger::logWarning(
        "Metal timestamp resolve failed for timeline {} (data {})",
        timelineValue, data ? data.length : 0);
    return false;
  }

  id<MTLDevice> metalDevice = static_cast<MetalDevice&>(device()).device();
  if (!metalDevice) {
    return false;
  }
  uint64_t frequency = [metalDevice queryTimestampFrequency];
  if (frequency == 0) {
    RengLogger::logWarning("Metal timestamp frequency invalid");
    return false;
  }

  const auto* entries =
      static_cast<const MTL4TimestampHeapEntry*>(data.bytes);
  double ticksToNs = 1e9 / static_cast<double>(frequency);
  timing.gpuStartNs =
      static_cast<uint64_t>(entries[0].timestamp * ticksToNs);
  timing.gpuEndNs =
      static_cast<uint64_t>(entries[1].timestamp * ticksToNs);
  timing.valid = true;
  entry.pending = false;
  _timestampHeapByValue.erase(it);
  return true;
}

void MetalCommandQueue::shutdownInner() {
  _timestampHeaps.clear();
  _timestampHeapByValue.clear();
  _queue = nil;
}

}  // namespace reng
