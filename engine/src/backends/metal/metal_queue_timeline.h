#pragma once

#import <Metal/Metal.h>

#include "reng/queue_timeline.h"

namespace reng {

class MetalQueueTimeline : public QueueTimeline {
 public:
  MetalQueueTimeline(BackendDevice& device) : QueueTimeline(device) {}
  ~MetalQueueTimeline() override = default;

  id<MTLSharedEvent> event() const { return _event; }
  uint64_t encodeSignal(id<MTLCommandBuffer> commandBuffer);

 protected:
  bool initInner() override final;
  void shutdownInner() override final;

 private:
  id<MTLSharedEvent> _event = nil;
};

}  // namespace reng
