#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "reng/app.h"

namespace reng::vulkan {

bool check(VkResult result, const char* msg);
VkFormat toVkFormat(PixelFormat format);
std::vector<const char*> gatherDeviceExtensions(VkPhysicalDevice device);
std::vector<const char*> gatherValidationLayers();

}  // namespace reng::vulkan
