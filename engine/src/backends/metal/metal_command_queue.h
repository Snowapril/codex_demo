#pragma once

#import <Metal/Metal.h>

#include "metal_queue_timeline.h"
#include "reng/command_queue.h"

namespace reng {

class MetalDevice;

class MetalCommandQueue : public CommandQueue {
 public:
  void shutdown() { CommandQueue::shutdown(); }

  id<MTLCommandQueue> queue() const { return _queue; }
  MetalQueueTimeline* metalTimeline() {
    return static_cast<MetalQueueTimeline*>(timeline());
  }

 private:
  std::unique_ptr<QueueTimeline> createTimeline(
      BackendDevice& device) override;
  bool initInner() override;
  void shutdownInner() override;

  id<MTLCommandQueue> _queue = nil;
};

}  // namespace reng
