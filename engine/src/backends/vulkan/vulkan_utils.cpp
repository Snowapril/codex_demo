#include "vulkan_utils.h"

#include "reng/logger.h"

namespace reng::vulkan {

bool check(VkResult result, const char* msg) {
  if (result != VK_SUCCESS) {
    RengLogger::logError("{} (VkResult={})", msg, result);
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

}  // namespace reng::vulkan
