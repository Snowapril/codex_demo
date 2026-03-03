#include "vulkan_device.h"

#import <QuartzCore/CAMetalLayer.h>

#include <array>
#include <string>
#include <vector>

#include "vulkan_utils.h"

namespace reng {

namespace {
std::vector<const char*> gatherInstanceExtensions() {
  std::vector<const char*> required = {VK_KHR_SURFACE_EXTENSION_NAME,
                                       VK_EXT_METAL_SURFACE_EXTENSION_NAME};
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> props(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

  std::vector<const char*> extensions;
  for (const auto& name : required) {
    bool found = false;
    for (const auto& prop : props) {
      if (name == std::string(prop.extensionName)) {
        found = true;
        break;
      }
    }
    if (!found) {
      return {};
    }
    extensions.push_back(name);
  }

  for (const auto& prop : props) {
    if (std::string(prop.extensionName) == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) {
      extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      break;
    }
  }

  return extensions;
}
}  // namespace

bool VulkanDevice::initMacos(void* metalLayer) {
  CAMetalLayer* layer = (__bridge CAMetalLayer*)metalLayer;
  if (!layer) {
    return false;
  }

  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Blank Vulkan Sample";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "reng";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  std::vector<const char*> extensions = gatherInstanceExtensions();
  if (extensions.empty()) {
    RengLogger::logError("Missing Vulkan instance extensions for Metal surface");
    return false;
  }

  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  for (auto name : extensions) {
    if (std::string(name) == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) {
      createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
      break;
    }
  }

  if (!vulkan::check(vkCreateInstance(&createInfo, nullptr, &_instance),
                     "vkCreateInstance failed")) {
    return false;
  }

  VkMetalSurfaceCreateInfoEXT surfaceInfo{
      VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
  surfaceInfo.pLayer = layer;

  auto createMetalSurface =
      reinterpret_cast<PFN_vkCreateMetalSurfaceEXT>(
          vkGetInstanceProcAddr(_instance, "vkCreateMetalSurfaceEXT"));
  if (!createMetalSurface) {
    shutdown();
    return false;
  }
  if (!vulkan::check(
          createMetalSurface(_instance, &surfaceInfo, nullptr, &_surface),
          "vkCreateMetalSurfaceEXT failed")) {
    shutdown();
    return false;
  }

  uint32_t count = 0;
  vkEnumeratePhysicalDevices(_instance, &count, nullptr);
  if (count == 0) {
    RengLogger::logError("No Vulkan physical devices found");
    shutdown();
    return false;
  }
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(_instance, &count, devices.data());
  _physicalDevice = devices[0];

  uint32_t queueCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> props(queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueCount,
                                           props.data());

  bool found = false;
  for (uint32_t i = 0; i < queueCount; ++i) {
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface,
                                         &presentSupport);
    if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
      _graphicsQueueFamily = i;
      found = true;
      break;
    }
  }
  if (!found) {
    RengLogger::logError("No graphics+present queue found");
    shutdown();
    return false;
  }

  float priority = 1.0f;
  VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueInfo.queueFamilyIndex = _graphicsQueueFamily;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priority;

  std::vector<const char*> deviceExtensions =
      vulkan::gatherDeviceExtensions(_physicalDevice);
  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (!vulkan::check(
          vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device),
          "vkCreateDevice failed")) {
    shutdown();
    return false;
  }

  vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);
  return true;
}

}  // namespace reng
