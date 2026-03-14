#pragma once

#import <Metal/Metal.h>

#include <unordered_map>
#include <vector>

#include "metal_queue_timeline.h"
#include "reng/command_queue.h"

namespace reng {

class MetalDevice;

class MetalCommandQueue : public CommandQueue {
 public:
  void shutdown() { CommandQueue::shutdown(); }

  id<MTL4CommandQueue> queue() const { return _queue; }
  MetalQueueTimeline* metalTimeline() {
    return static_cast<MetalQueueTimeline*>(timeline());
  }
  id<MTL4CounterHeap> acquireTimestampHeap(uint64_t timelineValue);
  bool resolveTimestamp(uint64_t timelineValue,
                        CommandBufferTiming& timing) override;

 private:
 std::unique_ptr<QueueTimeline> createTimeline(
      BackendDevice& device) override;
  std::unique_ptr<CommandBufferPool> createCommandBufferPool(
      BackendDevice& device) override;
  bool initInner() override;
  void shutdownInner() override;

  id<MTL4CommandQueue> _queue = nil;
  struct TimestampHeapEntry {
    id<MTL4CounterHeap> heap = nil;
    uint64_t lastValue = 0;
    bool pending = false;
  };
  std::vector<TimestampHeapEntry> _timestampHeaps;
  std::unordered_map<uint64_t, size_t> _timestampHeapByValue;
};

}  // namespace reng
