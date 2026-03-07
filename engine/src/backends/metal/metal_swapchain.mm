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
    id<CAMetalDrawable> drawable = [_layer nextDrawable];
    if (!drawable) {
      return;
    }

    MTL4RenderPassDescriptor* pass = [MTL4RenderPassDescriptor new];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.05, 0.05, 0.08, 1.0);

    if (!_presentQueue || !_presentQueue->queue()) {
      RengLogger::logError("Missing Metal present queue");
      return;
    }

    id<MTL4CommandBuffer> cmd = [_device.device() newCommandBuffer];
    id<MTL4CommandAllocator> allocator = [_device.device() newCommandAllocator];
    if (!cmd || !allocator) {
      RengLogger::logError("Failed to create Metal4 command buffer/allocator");
      return;
    }
    [cmd beginCommandBufferWithAllocator:allocator];
    id<MTL4RenderCommandEncoder> encoder =
        [cmd renderCommandEncoderWithDescriptor:pass];
    [encoder endEncoding];
    [cmd endCommandBuffer];
    id<MTL4CommandBuffer> buffers[] = {cmd};
    [_presentQueue->queue() commit:buffers count:1];
    [_presentQueue->queue() signalDrawable:drawable];
    [drawable present];
  }
}

}  // namespace reng
