#include "metal_swapchain.h"

#import <Metal/Metal.h>

#include "metal_utils.h"
#include "reng/logger.h"

namespace reng {

MetalSwapchain::MetalSwapchain(CAMetalLayer* layer, MetalDevice& device,
                               MetalCommandQueue* presentQueue,
                               const SwapchainDesc& desc)
    : _layer(layer),
      _device(device),
      _presentQueue(presentQueue),
      _desc(desc) {
  configureLayer(_desc);
}

bool MetalSwapchain::recreate(const SwapchainDesc& desc) {
  _desc = desc;
  configureLayer(_desc);
  return true;
}

ResourceId MetalSwapchain::acquireNextImage() {
  if (!_currentDrawable) {
    RengLogger::logError("Missing CAMetalDrawable before acquire");
  }
  return _swapchainResource;
}

void MetalSwapchain::signalPresentReady() {
  if (!_currentDrawable) {
    return;
  }
  if (!_presentQueue || !_presentQueue->queue()) {
    RengLogger::logError("Missing Metal present queue");
    return;
  }
  [_presentQueue->queue() signalDrawable:_currentDrawable];
}

void MetalSwapchain::configureLayer(const SwapchainDesc& desc) {
  _layer.device = _device.device();
  _layer.pixelFormat = metal::toMetalFormat(desc.colorFormat);
  _layer.framebufferOnly = YES;
  _layer.drawableSize = CGSizeMake(desc.width, desc.height);
  _size = {desc.width, desc.height};
  if (@available(macOS 12.0, *)) {
    _layer.displaySyncEnabled = (desc.presentMode == PresentMode::Vsync);
  }
}

void MetalSwapchain::present() {
  @autoreleasepool {
    if (!_currentDrawable) {
      return;
    }
    [_currentDrawable present];
    _currentDrawable = nil;
  }
}

}  // namespace reng
