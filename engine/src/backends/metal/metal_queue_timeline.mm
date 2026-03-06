#include "metal_queue_timeline.h"

#include "reng/logger.h"

namespace reng {

bool MetalQueueTimeline::init(id<MTLDevice> device) {
  if (!device) {
    RengLogger::logError("Missing Metal device for queue timeline");
    return false;
  }
  _event = [device newSharedEvent];
  if (!_event) {
    RengLogger::logError("Failed to create Metal shared event");
    return false;
  }
  _value = 0;
  return true;
}

void MetalQueueTimeline::shutdown() { _event = nil; }

uint64_t MetalQueueTimeline::encodeSignal(id<MTLCommandBuffer> commandBuffer) {
  if (!commandBuffer || !_event) {
    return _value;
  }
  uint64_t value = nextValue();
  [commandBuffer encodeSignalEvent:_event value:value];
  return value;
}

}  // namespace reng
