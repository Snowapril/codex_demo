#include "vulkan_device.h"

#include <array>
#include <algorithm>
#include <string>
#include <vector>

#include "reng/logger.h"
#include "vulkan_utils.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <vulkan/vulkan_core.h>
#endif

namespace reng {

namespace {

#if defined(_WIN32)
std::vector<const char*> gatherInstanceExtensions() {
  std::array<const char*, 2> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
  return std::vector<const char*>(extensions.begin(), extensions.end());
}
#elif defined(__APPLE__)
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

  return extensions;
}
#endif

}  // namespace

bool VulkanDevice::initDevice(void* param1, void* param2) {
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = _appName ? _appName : "reng";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "reng";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  std::vector<const char*> extensions = gatherInstanceExtensions();
  if (extensions.empty()) {
    RengLogger::logError("Required Vulkan instance extensions not available");
    return false;
  }
  std::vector<const char*> validationLayers;
  if (_desc.enableValidation) {
    validationLayers = vulkan::gatherValidationLayers();
  }

  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.enabledLayerCount =
      static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();

  if (!vulkan::check(vkCreateInstance(&createInfo, nullptr, &_instance),
                     "vkCreateInstance failed")) {
    return false;
  }

#if defined(_WIN32)
  HINSTANCE hinstance = static_cast<HINSTANCE>(param1);
  HWND hwnd = static_cast<HWND>(param2);
  if (!hinstance || !hwnd) {
    return false;
  }

  VkWin32SurfaceCreateInfoKHR surfaceInfo{
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
  surfaceInfo.hinstance = hinstance;
  surfaceInfo.hwnd = hwnd;
  if (!vulkan::check(vkCreateWin32SurfaceKHR(_instance, &surfaceInfo, nullptr,
                                             &_surface),
                     "vkCreateWin32SurfaceKHR failed")) {
    shutdown();
    return false;
  }
#elif defined(__APPLE__)
  void* metalLayer = param1;
  if (!metalLayer) {
    shutdown();
    return false;
  }
  RengLogger::logInfo(
      "macOS Vulkan instance created without portability enumeration; expecting KosmicKrisp");

  VkMetalSurfaceCreateInfoEXT surfaceInfo{
      VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
  surfaceInfo.pLayer = metalLayer;

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
#endif
  return initializeDevice();
}

bool VulkanDevice::initializeDevice() {
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

  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &familyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> props(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &familyCount,
                                           props.data());

  bool found = false;
  for (uint32_t i = 0; i < familyCount; ++i) {
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

  uint32_t desiredCopyCount = _desc.copyQueueCount > 0 ? _desc.copyQueueCount : 1;
  uint32_t availableQueues = props[_graphicsQueueFamily].queueCount;
  uint32_t queueCount = std::min(availableQueues, 1u + desiredCopyCount);
  std::vector<float> priorities(queueCount, 1.0f);

  VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueInfo.queueFamilyIndex = _graphicsQueueFamily;
  queueInfo.queueCount = queueCount;
  queueInfo.pQueuePriorities = priorities.data();

  std::vector<const char*> deviceExtensions =
      vulkan::gatherDeviceExtensions(_physicalDevice);

  VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
  VkPhysicalDeviceFeatures2 features2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  features2.pNext = &timelineFeatures;
  vkGetPhysicalDeviceFeatures2(_physicalDevice, &features2);
  if (!timelineFeatures.timelineSemaphore) {
    RengLogger::logError("Timeline semaphore feature not supported");
    shutdown();
    return false;
  }
  timelineFeatures.timelineSemaphore = VK_TRUE;

  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.pNext = &timelineFeatures;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (!vulkan::check(vkCreateDevice(_physicalDevice, &deviceInfo, nullptr,
                                    &_device),
                     "vkCreateDevice failed")) {
    shutdown();
    return false;
  }

  VkQueue graphicsQueue = VK_NULL_HANDLE;
  vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &graphicsQueue);
  _graphicsQueue = std::make_unique<VulkanCommandQueue>();
  _graphicsQueue->configure(graphicsQueue, _graphicsQueueFamily);
  if (!_graphicsQueue->init(*this, QueueType::Graphics)) {
    RengLogger::logError("Failed to initialize Vulkan graphics queue");
    return false;
  }

  _computeQueue = std::make_unique<VulkanCommandQueue>();
  _computeQueue->configure(graphicsQueue, _graphicsQueueFamily);
  if (!_computeQueue->init(*this, QueueType::Compute)) {
    RengLogger::logError("Failed to initialize Vulkan compute queue");
  }

  uint32_t copyCount = queueCount > 1 ? (queueCount - 1) : 0;
  if (copyCount == 0) {
    RengLogger::logWarning("Vulkan device provides a single queue; copy queues will share graphics queue");
    auto queue = std::make_unique<VulkanCommandQueue>();
    queue->configure(graphicsQueue, _graphicsQueueFamily);
    if (queue->init(*this, QueueType::Transfer)) {
      _copyQueues.push_back(std::move(queue));
    }
    return true;
  }

  _copyQueues.reserve(copyCount);
  for (uint32_t i = 0; i < copyCount; ++i) {
    VkQueue copyQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(_device, _graphicsQueueFamily, i + 1, &copyQueue);
    auto queue = std::make_unique<VulkanCommandQueue>();
    queue->configure(copyQueue, _graphicsQueueFamily);
    if (!queue->init(*this, QueueType::Transfer)) {
      RengLogger::logError("Failed to initialize Vulkan copy queue");
      continue;
    }
    _copyQueues.push_back(std::move(queue));
  }
  return true;
}

void VulkanDevice::shutdown() {
  for (auto& queue : _copyQueues) {
    if (queue) {
      queue->shutdown();
    }
  }
  _copyQueues.clear();
  if (_computeQueue) {
    _computeQueue->shutdown();
  }
  if (_graphicsQueue) {
    _graphicsQueue->shutdown();
  }
  _computeQueue.reset();
  _graphicsQueue.reset();
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

std::unique_ptr<CommandBuffer> VulkanDevice::createCommandBuffer(
    QueueType queueType) {
  return std::make_unique<VulkanCommandBuffer>(*this, queueType);
}

}  // namespace reng
