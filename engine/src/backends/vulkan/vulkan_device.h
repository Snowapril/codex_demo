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

#include <cstddef>
#include <memory>
#include <vector>

#include "reng/backend.h"
#include "reng/device.h"
#include "vulkan_command_queue.h"


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
  CommandQueue* graphicsQueue() const override {
    return _graphicsQueue.get();
  }
  CommandQueue* computeQueue() const override { return _computeQueue.get(); }
  size_t copyQueueCount() const override { return _copyQueues.size(); }
  CommandQueue* copyQueue(size_t index) const override {
    return index < _copyQueues.size() ? _copyQueues[index].get() : nullptr;
  }
  uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }
  double timestampPeriod() const { return _timestampPeriod; }
  uint32_t timestampValidBits() const { return _timestampValidBits; }

 private:
  bool initializeDevice();

  const char* _appName = nullptr;
  DeviceDesc _desc;
  VkInstance _instance = VK_NULL_HANDLE;
  VkSurfaceKHR _surface = VK_NULL_HANDLE;
  VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
  VkDevice _device = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
  uint32_t _graphicsQueueFamily = 0;
  uint32_t _timestampValidBits = 0;
  double _timestampPeriod = 0.0;
  std::unique_ptr<VulkanCommandQueue> _graphicsQueue;
  std::unique_ptr<VulkanCommandQueue> _computeQueue;
  std::vector<std::unique_ptr<VulkanCommandQueue>> _copyQueues;
};

}  // namespace reng
