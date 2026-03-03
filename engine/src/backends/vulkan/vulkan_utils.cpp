#include "vulkan_utils.h"

#include "reng/logger.h"

#include <string>

#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif

namespace reng::vulkan {

bool check(VkResult result, const char* msg) {
  if (result != VK_SUCCESS) {
    RengLogger::logError("{} (VkResult={})", msg,
                         static_cast<int>(result));
    return false;
  }
  return true;
}

VkFormat toVkFormat(PixelFormat format) {
  switch (format) {
    case PixelFormat::Bgra8Unorm:
    default:
      return VK_FORMAT_B8G8R8A8_UNORM;
  }
}

std::vector<const char*> gatherDeviceExtensions(VkPhysicalDevice device) {
  std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> props(count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, props.data());
  for (const auto& prop : props) {
    if (std::string(prop.extensionName) ==
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) {
      extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
      break;
    }
  }
  return extensions;
}

}  // namespace reng::vulkan
