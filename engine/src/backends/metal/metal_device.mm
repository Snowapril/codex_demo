#include "metal_device.h"

#include "reng/logger.h"

namespace reng {

MetalDevice::MetalDevice() {
  _device = MTLCreateSystemDefaultDevice();
  if (!_device) {
    RengLogger::logError("MTLCreateSystemDefaultDevice returned nil");
    return;
  }
  _queue = [_device newCommandQueue];
  if (!_queue) {
    RengLogger::logError("Failed to create Metal command queue");
  }
}

void MetalDevice::shutdown() {
  _queue = nil;
  _device = nil;
}

}  // namespace reng
