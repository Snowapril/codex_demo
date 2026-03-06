#pragma once

#import <QuartzCore/CAMetalLayer.h>

#include "reng/app.h"
#include "reng/backend.h"
#include "metal_device.h"

namespace reng {

class MetalSwapchain : public BackendSwapchain {
 public:
  MetalSwapchain(CAMetalLayer* layer, MetalDevice& device,
                 const SwapchainDesc& desc);
  bool recreate(const SwapchainDesc& desc) override;
  void present() override;
  uint32_t width() const override { return _desc.width; }
  uint32_t height() const override { return _desc.height; }
  PixelFormat colorFormat() const override { return _desc.colorFormat; }
  ResourceId acquireNextImage() override { return _swapchainResource; }

 private:
  void configureLayer(const SwapchainDesc& desc);

  CAMetalLayer* _layer = nil;
  MetalDevice& _device;
  SwapchainDesc _desc;
  ResourceId _swapchainResource{1, ResourceKind::Texture, "swapchain_color"};
};

}  // namespace reng
