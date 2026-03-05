#pragma once

#if defined(_WIN32) && !defined(VK_USE_PLATFORM_WIN32_KHR)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__APPLE__) && !defined(VK_USE_PLATFORM_METAL_EXT)
#define VK_USE_PLATFORM_METAL_EXT
#endif

#include <vulkan/vulkan.h>

#include "reng/backend.h"
#include "reng/device.h"


namespace reng {

class VulkanDevice : public BackendDevice {
 public:
  explicit VulkanDevice(const char* appName = nullptr,
                        const DeviceDesc& desc = DeviceDesc())
      : _appName(appName), _desc(desc) {}

  bool initDevice(void* param1, void* param2 = nullptr);
  void shutdown() override;

  VkInstance instance() const { return _instance; }
  VkSurfaceKHR surface() const { return _surface; }
  VkDevice device() const { return _device; }
  VkPhysicalDevice physicalDevice() const { return _physicalDevice; }
  VkQueue graphicsQueue() const { return _graphicsQueue; }
  uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }

 private:
  bool initializeDevice();

  const char* _appName = nullptr;
  DeviceDesc _desc;
  VkInstance _instance = VK_NULL_HANDLE;
  VkSurfaceKHR _surface = VK_NULL_HANDLE;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device = VK_NULL_HANDLE;
  VkQueue _graphicsQueue = VK_NULL_HANDLE;
  uint32_t _graphicsQueueFamily = 0;
};

}  // namespace reng
