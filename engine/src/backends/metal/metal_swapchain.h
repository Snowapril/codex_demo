#pragma once

#import <QuartzCore/CAMetalLayer.h>

#include "reng/app.h"
#include "reng/backend.h"
#include "metal_device.h"

namespace reng {

class MetalSwapchain : public Swapchain {
 public:
  MetalSwapchain(CAMetalLayer* layer, MetalDevice& device,
                 const SwapchainDesc& desc);
  void recreate(const SwapchainDesc& desc) override;
  void present() override;

 private:
  void configureLayer(const SwapchainDesc& desc);

  CAMetalLayer* _layer = nil;
  MetalDevice& _device;
  SwapchainDesc _desc;
};

}  // namespace reng
