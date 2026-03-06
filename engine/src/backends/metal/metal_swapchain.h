#pragma once

#import <QuartzCore/CAMetalLayer.h>

#include "reng/app.h"
#include "reng/backend.h"
#include "metal_device.h"

namespace reng {

class MetalSwapchain : public BackendSwapchain {
 public:
  MetalSwapchain(CAMetalLayer* layer, MetalDevice& device,
                 id<MTLCommandQueue> presentQueue,
                 const SwapchainDesc& desc);
  bool recreate(const SwapchainDesc& desc) override;
  void present() override;
  PixelFormat colorFormat() const override { return _desc.colorFormat; }
  ResourceId acquireNextImage() override { return _swapchainResource; }

 private:
  void configureLayer(const SwapchainDesc& desc);

  CAMetalLayer* _layer = nil;
  MetalDevice& _device;
  id<MTLCommandQueue> _presentQueue = nil;
  SwapchainDesc _desc;
  ResourceId _swapchainResource{1, ResourceKind::Texture, "swapchain_color"};
};

}  // namespace reng
