#import <QuartzCore/CAMetalLayer.h>

#include "engine/src/platform/backend_factory.h"
#include "reng/logger.h"

#include "backends/metal/metal_device.h"
#include "backends/metal/metal_swapchain.h"

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
    auto device = std::make_unique<MetalDevice>();
    auto swapchain =
        std::make_unique<MetalSwapchain>(layer, *device, desc.swapchain);
    bundle.device = std::move(device);
    bundle.swapchain = std::move(swapchain);
    return bundle;
  }

  if (desc.backend == Backend::Vulkan) {
    RengLogger::logError(
        "Vulkan backend on macOS is not wired yet (KosmicKrisp pending)");
  }
  return bundle;
}

}  // namespace reng
