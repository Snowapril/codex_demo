#include "platform/backend_factory.h"

#include "reng/logger.h"

#include "backends/vulkan/vulkan_device.h"
#include "backends/vulkan/vulkan_resources.h"
#include "backends/vulkan/vulkan_swapchain.h"

namespace reng {

BackendBundle createBackend(const AppDesc& desc, const PlatformContext& context) {
  BackendBundle bundle;
  if (context.platform != PlatformKind::Windows) {
    RengLogger::logError("Windows backend factory used on non-Windows platform");
    return bundle;
  }

  if (desc.backend != Backend::Vulkan) {
    RengLogger::logError("Requested backend not supported on Windows");
    return bundle;
  }

  HINSTANCE hinstance = static_cast<HINSTANCE>(context.windows.hinstance);
  HWND hwnd = static_cast<HWND>(context.windows.hwnd);
  if (!hinstance || !hwnd) {
    RengLogger::logError("Missing HWND/HINSTANCE for Vulkan backend");
    return bundle;
  }

  auto device = std::make_unique<VulkanDevice>(desc.title, desc.device);
  if (!device->initDevice(hinstance, hwnd)) {
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
}

}  // namespace reng
