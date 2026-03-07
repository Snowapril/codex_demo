#import <QuartzCore/CAMetalLayer.h>

#include "platform/backend_factory.h"
#include "reng/logger.h"

#include "backends/metal/metal_device.h"
#include "backends/metal/metal_resources.h"
#include "backends/metal/metal_swapchain.h"

#if defined(RENG_ENABLE_VULKAN)
#include "backends/vulkan/vulkan_device.h"
#include "backends/vulkan/vulkan_resources.h"
#include "backends/vulkan/vulkan_swapchain.h"
#endif

namespace reng {

BackendBundle createBackend(const AppDesc& desc, const PlatformContext& context) {
  BackendBundle bundle;
  if (context.platform != PlatformKind::MacOS) {
    RengLogger::logError("macOS backend factory used on non-macOS platform");
    return bundle;
  }

  if (desc.backend == Backend::Metal) {
    CAMetalLayer* layer =
        (__bridge CAMetalLayer*)context.macos.metalLayer;
    if (!layer) {
      RengLogger::logError("Missing CAMetalLayer for Metal backend");
      return bundle;
    }
    auto device = std::make_unique<MetalDevice>(desc.device);
    auto swapchain = std::make_unique<MetalSwapchain>(
        layer, *device,
        static_cast<MetalCommandQueue*>(device->graphicsQueue()),
        desc.swapchain);
    auto resources = std::make_unique<MetalResources>();
    bundle.device = std::move(device);
    bundle.swapchain = std::move(swapchain);
    bundle.resources = std::move(resources);
    return bundle;
  }

  if (desc.backend == Backend::Vulkan) {
#if defined(RENG_ENABLE_VULKAN)
    CAMetalLayer* layer =
        (__bridge CAMetalLayer*)context.macos.metalLayer;
    if (!layer) {
      RengLogger::logError("Missing CAMetalLayer for Vulkan backend");
      return bundle;
    }
    auto device = std::make_unique<VulkanDevice>(desc.title, desc.device);
    if (!device->initDevice((__bridge void*)layer)) {
      RengLogger::logError("Failed to initialize Vulkan device");
      return bundle;
    }
    auto swapchain = std::make_unique<VulkanSwapchain>();
    if (!swapchain->init(
            *device,
            static_cast<VulkanCommandQueue*>(device->graphicsQueue()),
            desc.swapchain)) {
      RengLogger::logError("Failed to initialize Vulkan swapchain");
      device->shutdown();
      return bundle;
    }
    auto resources = std::make_unique<VulkanResources>();
    bundle.device = std::move(device);
    bundle.swapchain = std::move(swapchain);
    bundle.resources = std::move(resources);
    return bundle;
#else
    RengLogger::logError("Vulkan backend is disabled in this build");
#endif
  }
  return bundle;
}

}  // namespace reng
