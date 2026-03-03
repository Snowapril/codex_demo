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


namespace reng {

class VulkanDevice : public BackendDevice {
 public:
  explicit VulkanDevice(const char* appName = nullptr) : _appName(appName) {}

  bool initWin32(void* hinstance, void* hwnd);
  bool initMacos(void* metalLayer);
  void shutdown() override;

  VkInstance instance() const { return _instance; }
  VkSurfaceKHR surface() const { return _surface; }
  VkDevice device() const { return _device; }
  VkPhysicalDevice physicalDevice() const { return _physicalDevice; }
  VkQueue graphicsQueue() const { return _graphicsQueue; }
  uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }

 private:
  const char* _appName = nullptr;
  VkInstance _instance = VK_NULL_HANDLE;
  VkSurfaceKHR _surface = VK_NULL_HANDLE;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device = VK_NULL_HANDLE;
  VkQueue _graphicsQueue = VK_NULL_HANDLE;
  uint32_t _graphicsQueueFamily = 0;
};

}  // namespace reng
