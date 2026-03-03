#pragma once

#include <vulkan/vulkan.h>

#include "reng/app.h"

namespace reng::vulkan {

bool check(VkResult result, const char* msg);
VkFormat toVkFormat(PixelFormat format);

}  // namespace reng::vulkan
