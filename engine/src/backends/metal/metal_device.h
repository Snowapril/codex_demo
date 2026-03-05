#pragma once

#import <Metal/Metal.h>

#include "reng/backend.h"

namespace reng {

class MetalDevice : public BackendDevice {
 public:
  MetalDevice();
  void shutdown() override;
  id<MTLDevice> device() const { return _device; }
  id<MTLCommandQueue> queue() const { return _queue; }

 private:
  id<MTLDevice> _device = nil;
  id<MTLCommandQueue> _queue = nil;
};

}  // namespace reng
