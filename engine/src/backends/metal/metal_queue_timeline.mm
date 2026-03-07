#include "metal_queue_timeline.h"

#include "metal_device.h"
#include "reng/logger.h"

namespace reng {

bool MetalQueueTimeline::initInner() {
  id<MTLDevice> device =
      static_cast<MetalDevice&>(_device).device();
  _event = [device newSharedEvent];
  if (!_event) {
    RengLogger::logError("Failed to create Metal shared event");
    return false;
  }
  return true;
}

void MetalQueueTimeline::shutdownInner() { _event = nil; }

}  // namespace reng
