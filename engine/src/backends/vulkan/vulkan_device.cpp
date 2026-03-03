#include "vulkan_device.h"

#if defined(_WIN32)
#include <array>
#include <string>
#include <vector>

#include "reng/logger.h"
#include "vulkan_utils.h"

namespace reng {
bool VulkanDevice::initWin32(void* hinstance, void* hwnd) {
  HINSTANCE hinst = static_cast<HINSTANCE>(hinstance);
  HWND window = static_cast<HWND>(hwnd);
  if (!hinst || !window) {
    return false;
  }
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = _appName ? _appName : "reng";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "reng";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  std::array<const char*, 2> extensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                           VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (!vulkan::check(vkCreateInstance(&createInfo, nullptr, &_instance),
             "vkCreateInstance failed")) {
    return false;
  }

  VkWin32SurfaceCreateInfoKHR surfaceInfo{
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
  surfaceInfo.hinstance = hinst;
  surfaceInfo.hwnd = window;
  if (!vulkan::check(vkCreateWin32SurfaceKHR(_instance, &surfaceInfo, nullptr, &_surface),
             "vkCreateWin32SurfaceKHR failed")) {
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
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueCount, nullptr);
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

  std::vector<const char*> extensions =
      vulkan::gatherDeviceExtensions(_physicalDevice);
  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  deviceInfo.ppEnabledExtensionNames = extensions.data();

  if (!vulkan::check(vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device),
             "vkCreateDevice failed")) {
    shutdown();
    return false;
  }

  vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);
  return true;
}

void VulkanDevice::shutdown() {
  if (_device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(_device);
    vkDestroyDevice(_device, nullptr);
    _device = VK_NULL_HANDLE;
  }
  if (_surface != VK_NULL_HANDLE && _instance != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    _surface = VK_NULL_HANDLE;
  }
  if (_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(_instance, nullptr);
    _instance = VK_NULL_HANDLE;
  }
}

}  // namespace reng
#endif
