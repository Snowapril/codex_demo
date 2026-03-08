#pragma once

#import <Metal/Metal.h>

#include "reng/queue_timeline.h"

namespace reng {

class MetalQueueTimeline : public QueueTimeline {
 public:
  MetalQueueTimeline(BackendDevice& device) : QueueTimeline(device) {}
  ~MetalQueueTimeline() override = default;

  id<MTLSharedEvent> event() const { return _event; }
  void signalQueue(id<MTL4CommandQueue> queue, uint64_t value);
  uint64_t completedValue() const override;

 protected:
  bool initInner() override final;
  void shutdownInner() override final;

 private:
  id<MTLSharedEvent> _event = nil;
};

}  // namespace reng
