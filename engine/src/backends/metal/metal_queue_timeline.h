#pragma once

#import <Metal/Metal.h>

#include "reng/queue_timeline.h"

namespace reng {

class MetalQueueTimeline : public QueueTimeline {
 public:
  bool init(id<MTLDevice> device);
  void shutdown();

  id<MTLSharedEvent> event() const { return _event; }
  uint64_t encodeSignal(id<MTLCommandBuffer> commandBuffer);

 private:
  id<MTLSharedEvent> _event = nil;
};

}  // namespace reng
