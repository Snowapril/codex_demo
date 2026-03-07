#pragma once

#import <QuartzCore/CAMetalLayer.h>

#include "reng/app.h"
#include "reng/backend.h"
#include "metal_command_queue.h"
#include "metal_device.h"

namespace reng {

class MetalSwapchain : public BackendSwapchain {
 public:
  MetalSwapchain(CAMetalLayer* layer, MetalDevice& device,
                 MetalCommandQueue* presentQueue,
                 const SwapchainDesc& desc);
  bool recreate(const SwapchainDesc& desc) override;
  void signalPresentReady() override;
  void present() override;
  PixelFormat colorFormat() const override { return _desc.colorFormat; }
  ResourceId acquireNextImage() override;

 private:
  void configureLayer(const SwapchainDesc& desc);

  CAMetalLayer* _layer = nil;
  MetalDevice& _device;
  MetalCommandQueue* _presentQueue = nullptr;
  id<CAMetalDrawable> _currentDrawable = nil;
  SwapchainDesc _desc;
  ResourceId _swapchainResource{1, ResourceKind::Texture, "swapchain_color"};
};

}  // namespace reng
