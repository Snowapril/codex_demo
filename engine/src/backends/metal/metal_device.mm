#include "metal_device.h"

namespace reng {

MetalDevice::MetalDevice() {
  _device = MTLCreateSystemDefaultDevice();
  _queue = [_device newCommandQueue];
}

void MetalDevice::shutdown() {
  _queue = nil;
  _device = nil;
}

}  // namespace reng
