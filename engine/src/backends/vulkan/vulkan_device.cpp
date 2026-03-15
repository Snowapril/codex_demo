#include "vulkan_device.h"

#include <array>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "reng/logger.h"
#include "vulkan_utils.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <vulkan/vulkan_core.h>
#include <unistd.h>
#endif


namespace reng {

namespace {

#if defined(RENG_ENABLE_VULKAN)
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData) {
  (void)type;
  (void)userData;
  if (!callbackData || !callbackData->pMessage) {
    return VK_FALSE;
  }

  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    RengLogger::logError("Vulkan validation: {}", callbackData->pMessage);
  } else if (severity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    RengLogger::logWarning("Vulkan validation: {}", callbackData->pMessage);
  } else if (severity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    RengLogger::logInfo("Vulkan validation: {}", callbackData->pMessage);
  } else {
    RengLogger::logVerbose("Vulkan validation: {}", callbackData->pMessage);
  }
  return VK_FALSE;
}
#endif

bool hasInstanceExtension(const char* name) {
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> props(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());
  for (const auto& prop : props) {
    if (name == std::string(prop.extensionName)) {
      return true;
    }
  }
  return false;
}

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
  RengLogger::logInfo("Vulkan init: validation {}", _desc.enableValidation ? "on" : "off");
#if defined(__APPLE__)
  const char* icdEnv = getenv("VK_ICD_FILENAMES");
  if (!icdEnv || icdEnv[0] == '\0') {
    const char* candidates[] = {
        "/usr/local/share/vulkan/icd.d/libkosmickrisp_icd.json",
        "/opt/homebrew/share/vulkan/icd.d/libkosmickrisp_icd.json"};
    for (const char* path : candidates) {
      if (access(path, F_OK) == 0) {
        setenv("VK_ICD_FILENAMES", path, 0);
        RengLogger::logInfo("VK_ICD_FILENAMES set to {}", path);
        break;
      }
    }
  }
#endif
  uint32_t instanceVersion = VK_API_VERSION_1_0;
  auto enumerateVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
      vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
  if (enumerateVersion) {
    enumerateVersion(&instanceVersion);
  }
  uint32_t requestedVersion = VK_API_VERSION_1_3;
  uint32_t apiVersion =
      instanceVersion < requestedVersion ? instanceVersion : requestedVersion;

  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = _appName ? _appName : "reng";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "reng";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = apiVersion;

  std::vector<const char*> extensions = gatherInstanceExtensions();
  if (extensions.empty()) {
    RengLogger::logError("Required Vulkan instance extensions not available");
    return false;
  }
  std::vector<const char*> validationLayers;
  if (_desc.enableValidation) {
    validationLayers = vulkan::gatherValidationLayers();
    if (hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    } else {
      RengLogger::logWarning(
          "Vulkan debug utils extension not available");
    }
    if (validationLayers.empty()) {
      RengLogger::logWarning(
          "Vulkan validation enabled but no validation layers found");
    } else {
      for (const auto* layer : validationLayers) {
        RengLogger::logInfo("Vulkan validation layer: {}", layer);
      }
    }
    for (const auto* ext : extensions) {
      RengLogger::logVerbose("Vulkan instance extension: {}", ext);
    }
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

  if (_desc.enableValidation) {
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = debugCallback;

    auto createDebugMessenger =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (createDebugMessenger) {
      if (!vulkan::check(
              createDebugMessenger(_instance, &debugInfo, nullptr,
                                   &_debugMessenger),
              "vkCreateDebugUtilsMessengerEXT failed")) {
        _debugMessenger = VK_NULL_HANDLE;
      } else {
        RengLogger::logInfo("Vulkan debug messenger created");
      }
    } else {
      RengLogger::logWarning(
          "Vulkan debug utils extension not available");
    }
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
  RengLogger::logInfo("macOS Vulkan instance created (KosmicKrisp)");

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
    const char* icdEnv = getenv("VK_ICD_FILENAMES");
    if (icdEnv && icdEnv[0] != '\0') {
      RengLogger::logWarning("VK_ICD_FILENAMES={}", icdEnv);
    } else {
      RengLogger::logWarning("VK_ICD_FILENAMES not set");
    }
    RengLogger::logError("No Vulkan physical devices found");
    shutdown();
    return false;
  }
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(_instance, &count, devices.data());
  
  _physicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties selectedProps{};
  for ( const auto& device : devices) {
    VkPhysicalDeviceProperties deviceProps;
    vkGetPhysicalDeviceProperties(device, &deviceProps);
    RengLogger::logInfo("Vulkan physical device: {} (API version {})",
                         deviceProps.deviceName,
                         VK_VERSION_MAJOR(deviceProps.apiVersion));
    if (deviceProps.apiVersion < VK_API_VERSION_1_4) {
      continue;
    }

    if ( deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      _physicalDevice = device;
      selectedProps = deviceProps;
      break;
    }
  }

  // TODO : also check fall-back device support vulkan 1.4 features
  if (_physicalDevice == VK_NULL_HANDLE) {
      RengLogger::logWarning("No discrete GPU found, using first available device");
      _physicalDevice = devices[0];
  }

  if (_physicalDevice != VK_NULL_HANDLE) {
    vkGetPhysicalDeviceProperties(_physicalDevice, &selectedProps);
    _timestampPeriod =
        static_cast<double>(selectedProps.limits.timestampPeriod);
  }

  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &familyCount,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> props(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &familyCount,
                                           props.data());

  // Pick a queue family that supports graphics and present (ideally the same one for simplicity) for graphics queue family.
  // If multiple queues are available in the same family, we will use them for copy queues. If no separate copy queue family is available, copy queues will share the graphics queue family.
  // Also check compute only queue family for async compute if available, but if no separate compute queue is available, compute will share the graphics queue family as well.
  const uint32_t kInvalidFamily = UINT32_MAX;
  uint32_t graphicsFamily = kInvalidFamily;
  uint32_t computeFamily = kInvalidFamily;
  uint32_t transferFamily = kInvalidFamily;

  for (uint32_t i = 0; i < familyCount; ++i) {
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface,
                                         &presentSupport);
    const VkQueueFlags flags = props[i].queueFlags;
    const bool hasGraphics = (flags & VK_QUEUE_GRAPHICS_BIT) != 0;
    const bool hasCompute = (flags & VK_QUEUE_COMPUTE_BIT) != 0;
    const bool hasTransfer = (flags & VK_QUEUE_TRANSFER_BIT) != 0;

    if (graphicsFamily == kInvalidFamily && hasGraphics && presentSupport) {
      graphicsFamily = i;
      _timestampValidBits = props[i].timestampValidBits;
    }
    if (computeFamily == kInvalidFamily && hasCompute && !hasGraphics) {
      computeFamily = i;
    }
    if (transferFamily == kInvalidFamily && hasTransfer && !hasGraphics &&
        !hasCompute) {
      transferFamily = i;
    }
  }

  if (graphicsFamily == kInvalidFamily) {
    RengLogger::logError("No graphics+present queue found");
    shutdown();
    return false;
  }

  if (computeFamily == kInvalidFamily) {
    computeFamily = graphicsFamily;
  }
  if (transferFamily == kInvalidFamily) {
    transferFamily = graphicsFamily;
  }

  _queueFamilies[static_cast<size_t>(QueueType::Graphics)] = graphicsFamily;
  _queueFamilies[static_cast<size_t>(QueueType::Compute)] = computeFamily;
  _queueFamilies[static_cast<size_t>(QueueType::Transfer)] = transferFamily;

  uint32_t desiredCopyCount =
      _desc.copyQueueCount > 0 ? _desc.copyQueueCount : 1;
  uint32_t graphicsQueues =
      props[this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)]]
          .queueCount;
  uint32_t computeQueues =
      props[this->_queueFamilies[static_cast<size_t>(QueueType::Compute)]]
          .queueCount;
  uint32_t transferQueues =
      props[this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)]]
          .queueCount;

  bool transferSharesGraphics =
      this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)] ==
      this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)];
  bool computeSharesGraphics =
      this->_queueFamilies[static_cast<size_t>(QueueType::Compute)] ==
      this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)];

  uint32_t copyCount = 0;
  uint32_t graphicsQueueCount = 1;
  if (transferSharesGraphics) {
    uint32_t maxCopy = graphicsQueues > 1 ? (graphicsQueues - 1) : 0;
    copyCount = (std::min)(desiredCopyCount, maxCopy);
    graphicsQueueCount = 1 + copyCount;
  } else {
    copyCount = (std::min)(desiredCopyCount, transferQueues);
    if (copyCount == 0) {
      RengLogger::logWarning(
          "No dedicated transfer queues available; using graphics queue");
      this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)] =
          this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)];
      transferSharesGraphics = true;
      uint32_t maxCopy = graphicsQueues > 1 ? (graphicsQueues - 1) : 0;
      copyCount = (std::min)(desiredCopyCount, maxCopy);
      graphicsQueueCount = 1 + copyCount;
    }
  }

  if (!computeSharesGraphics && computeQueues == 0) {
    this->_queueFamilies[static_cast<size_t>(QueueType::Compute)] =
        this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)];
    computeSharesGraphics = true;
  }

  std::vector<VkDeviceQueueCreateInfo> queueInfos;
  std::vector<std::vector<float>> queuePriorities;
  queueInfos.reserve(3);
  queuePriorities.reserve(3);

  auto addQueueInfo = [&](uint32_t familyIndex, uint32_t count) {
    if (count == 0) {
      return;
    }
    queuePriorities.emplace_back(count, 1.0f);
    VkDeviceQueueCreateInfo info{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    info.queueFamilyIndex = familyIndex;
    info.queueCount = count;
    info.pQueuePriorities = queuePriorities.back().data();
    queueInfos.push_back(info);
  };

  addQueueInfo(this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)],
               graphicsQueueCount);
  if (!computeSharesGraphics) {
    addQueueInfo(this->_queueFamilies[static_cast<size_t>(QueueType::Compute)],
                 1);
  }
  if (!transferSharesGraphics) {
    addQueueInfo(this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)],
                 copyCount);
  }

  std::vector<const char*> deviceExtensions =
      vulkan::gatherDeviceExtensions(_physicalDevice);

  VkPhysicalDeviceDynamicRenderingFeatures dynamicFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
  VkPhysicalDeviceTimelineSemaphoreFeatures timelineFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
  VkPhysicalDeviceFeatures2 features2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  features2.pNext = &dynamicFeatures;
  dynamicFeatures.pNext = &timelineFeatures;
  vkGetPhysicalDeviceFeatures2(_physicalDevice, &features2);
  if (!timelineFeatures.timelineSemaphore) {
    RengLogger::logError("Timeline semaphore feature not supported");
    shutdown();
    return false;
  }
  if (!dynamicFeatures.dynamicRendering) {
    RengLogger::logError("Dynamic rendering feature not supported");
    shutdown();
    return false;
  }
  timelineFeatures.timelineSemaphore = VK_TRUE;
  dynamicFeatures.dynamicRendering = VK_TRUE;

  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.pNext = &dynamicFeatures;
  deviceInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueInfos.size());
  deviceInfo.pQueueCreateInfos = queueInfos.data();
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
  vkGetDeviceQueue(
      _device, this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)],
      0, &graphicsQueue);
  _graphicsQueue = std::make_unique<VulkanCommandQueue>();
  _graphicsQueue->configure(
      graphicsQueue,
      this->_queueFamilies[static_cast<size_t>(QueueType::Graphics)]);
  if (!_graphicsQueue->init(*this, QueueType::Graphics)) {
    RengLogger::logError("Failed to initialize Vulkan graphics queue");
    return false;
  }

  VkQueue computeQueueHandle = graphicsQueue;
  if (!computeSharesGraphics) {
    vkGetDeviceQueue(
        _device, this->_queueFamilies[static_cast<size_t>(QueueType::Compute)],
        0, &computeQueueHandle);
  }
  _computeQueue = std::make_unique<VulkanCommandQueue>();
  _computeQueue->configure(
      computeQueueHandle,
      this->_queueFamilies[static_cast<size_t>(QueueType::Compute)]);
  if (!_computeQueue->init(*this, QueueType::Compute)) {
    RengLogger::logError("Failed to initialize Vulkan compute queue");
  }

  if (copyCount == 0) {
    RengLogger::logWarning("Vulkan device provides a single queue; copy queues will share graphics queue");
    auto queue = std::make_unique<VulkanCommandQueue>();
    queue->configure(
        graphicsQueue,
        this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)]);
    if (queue->init(*this, QueueType::Transfer)) {
      _copyQueues.push_back(std::move(queue));
    }
    return true;
  }

  _copyQueues.reserve(copyCount);
  for (uint32_t i = 0; i < copyCount; ++i) {
    VkQueue copyQueue = VK_NULL_HANDLE;
    uint32_t queueIndex = transferSharesGraphics ? (i + 1) : i;
    vkGetDeviceQueue(
        _device,
        this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)],
        queueIndex, &copyQueue);
    auto queue = std::make_unique<VulkanCommandQueue>();
    queue->configure(
        copyQueue,
        this->_queueFamilies[static_cast<size_t>(QueueType::Transfer)]);
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
    if (_debugMessenger != VK_NULL_HANDLE) {
      auto destroyDebugMessenger =
          reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
              vkGetInstanceProcAddr(_instance,
                                    "vkDestroyDebugUtilsMessengerEXT"));
      if (destroyDebugMessenger) {
        destroyDebugMessenger(_instance, _debugMessenger, nullptr);
      }
      _debugMessenger = VK_NULL_HANDLE;
    }
    vkDestroyInstance(_instance, nullptr);
    _instance = VK_NULL_HANDLE;
  }
}

}  // namespace reng
