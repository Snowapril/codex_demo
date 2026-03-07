#include "vulkan_swapchain.h"

#include <algorithm>

#include "vulkan_utils.h"

#include "reng/logger.h"

namespace reng {

bool VulkanSwapchain::init(VulkanDevice& device, const SwapchainDesc& desc) {
  return init(device,
              static_cast<VulkanCommandQueue*>(device.graphicsQueue()), desc);
}

bool VulkanSwapchain::init(VulkanDevice& device, VulkanCommandQueue* presentQueue,
                           const SwapchainDesc& desc) {
  _device = &device;
  _presentQueue = presentQueue;
  return createSwapchainResources(desc);
}

bool VulkanSwapchain::recreate(const SwapchainDesc& desc) {
  if (!_device) {
    return false;
  }
  VkDevice device = _device->device();
  vkDeviceWaitIdle(device);
  destroySwapchainResources(device);
  return createSwapchainResources(desc);
}

void VulkanSwapchain::shutdown(VkDevice device) {
  destroySwapchainResources(device);
  _device = nullptr;
}

void VulkanSwapchain::present() {
  if (!_device) {
    return;
  }
  VkDevice device = _device->device();
  if (!_presentQueue || _presentQueue->queue() == VK_NULL_HANDLE) {
    RengLogger::logError("Missing Vulkan present queue");
    return;
  }

  if (!_hasAcquiredImage) {
    RengLogger::logError("Vulkan present called without acquired image");
    return;
  }

  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 0;
  presentInfo.pWaitSemaphores = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &_swapchain;
  presentInfo.pImageIndices = &_acquiredImageIndex;
  vkQueuePresentKHR(_presentQueue->queue(), &presentInfo);

  _hasAcquiredImage = false;
}

PixelFormat VulkanSwapchain::colorFormat() const {
  return vulkan::fromVkFormat(_format);
}

ResourceId VulkanSwapchain::acquireNextImage() {
  if (!_device) {
    return _swapchainResource;
  }
  VkDevice device = _device->device();
  vkWaitForFences(device, 1, &_inFlight, VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &_inFlight);

  if (!vulkan::check(vkAcquireNextImageKHR(device, _swapchain, UINT64_MAX,
                                           VK_NULL_HANDLE, _inFlight,
                                           &_acquiredImageIndex),
                     "vkAcquireNextImageKHR failed")) {
    _hasAcquiredImage = false;
    return _swapchainResource;
  }
  _hasAcquiredImage = true;
  return _swapchainResource;
}

bool VulkanSwapchain::createSwapchainResources(const SwapchainDesc& desc) {
  if (!_device) {
    return false;
  }
  VulkanDevice& device = *_device;
  VkSurfaceCapabilitiesKHR caps{};
  if (!vulkan::check(
          vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
              device.physicalDevice(), device.surface(), &caps),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed")) {
    return false;
  }

  uint32_t formatCount = 0;
  if (!vulkan::check(
          vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice(),
                                               device.surface(),
                                               &formatCount, nullptr),
          "vkGetPhysicalDeviceSurfaceFormatsKHR failed")) {
    return false;
  }
  if (formatCount == 0) {
    RengLogger::logError("No Vulkan surface formats available");
    return false;
  }
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  if (!vulkan::check(
          vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice(),
                                               device.surface(),
                                               &formatCount, formats.data()),
          "vkGetPhysicalDeviceSurfaceFormatsKHR failed")) {
    return false;
  }

  VkSurfaceFormatKHR chosen = formats[0];
  VkFormat desired = vulkan::toVkFormat(desc.colorFormat);
  for (const auto& fmt : formats) {
    if (fmt.format == desired &&
        fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      chosen = fmt;
      break;
    }
  }

  _format = chosen.format;
  if (caps.currentExtent.width != UINT32_MAX) {
    _extent = caps.currentExtent;
  } else {
    _extent = VkExtent2D{
        std::clamp(desc.width, caps.minImageExtent.width,
                   caps.maxImageExtent.width),
        std::clamp(desc.height, caps.minImageExtent.height,
                   caps.maxImageExtent.height)};
  }
  _size = {static_cast<uint32_t>(_extent.width),
           static_cast<uint32_t>(_extent.height)};

  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
    imageCount = caps.maxImageCount;
  }

  VkPresentModeKHR desiredPresent = VK_PRESENT_MODE_FIFO_KHR;
  if (desc.presentMode == PresentMode::Immediate) {
    desiredPresent = VK_PRESENT_MODE_IMMEDIATE_KHR;
  }

  uint32_t presentCount = 0;
  if (!vulkan::check(
          vkGetPhysicalDeviceSurfacePresentModesKHR(
              device.physicalDevice(), device.surface(), &presentCount,
              nullptr),
          "vkGetPhysicalDeviceSurfacePresentModesKHR failed")) {
    return false;
  }
  if (presentCount == 0) {
    RengLogger::logError("No Vulkan present modes available");
    return false;
  }
  std::vector<VkPresentModeKHR> presentModes(presentCount);
  if (!vulkan::check(
          vkGetPhysicalDeviceSurfacePresentModesKHR(
              device.physicalDevice(), device.surface(), &presentCount,
              presentModes.data()),
          "vkGetPhysicalDeviceSurfacePresentModesKHR failed")) {
    return false;
  }

  VkPresentModeKHR selectedPresent = VK_PRESENT_MODE_FIFO_KHR;
  for (auto mode : presentModes) {
    if (mode == desiredPresent) {
      selectedPresent = mode;
      break;
    }
  }

  VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  info.surface = device.surface();
  info.minImageCount = imageCount;
  info.imageFormat = _format;
  info.imageColorSpace = chosen.colorSpace;
  info.imageExtent = _extent;
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.preTransform = caps.currentTransform;
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.presentMode = selectedPresent;
  info.clipped = VK_TRUE;

  auto fail = [&]() {
    destroySwapchainResources(device.device());
    return false;
  };

  if (!vulkan::check(
          vkCreateSwapchainKHR(device.device(), &info, nullptr, &_swapchain),
          "vkCreateSwapchainKHR failed")) {
    return fail();
  }

  if (!vulkan::check(
          vkGetSwapchainImagesKHR(device.device(), _swapchain, &imageCount,
                                  nullptr),
          "vkGetSwapchainImagesKHR(count) failed")) {
    RengLogger::logError("Failed to query Vulkan swapchain image count");
    return fail();
  }
  if (imageCount == 0) {
    RengLogger::logError("vkGetSwapchainImagesKHR returned zero images");
    return fail();
  }
  _images.resize(imageCount);
  if (!vulkan::check(
          vkGetSwapchainImagesKHR(device.device(), _swapchain, &imageCount,
                                  _images.data()),
          "vkGetSwapchainImagesKHR(images) failed")) {
    RengLogger::logError("Failed to fetch Vulkan swapchain images");
    return fail();
  }

  _imageViews.resize(_images.size());
  for (size_t i = 0; i < _images.size(); ++i) {
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = _images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = _format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (!vulkan::check(vkCreateImageView(device.device(), &viewInfo, nullptr,
                                         &_imageViews[i]),
                       "vkCreateImageView failed")) {
      return fail();
    }
  }

  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (!vulkan::check(
          vkCreateFence(device.device(), &fenceInfo, nullptr, &_inFlight),
          "vkCreateFence failed")) {
    return fail();
  }

  return true;
}

void VulkanSwapchain::destroySwapchainResources(VkDevice device) {
  if (device != VK_NULL_HANDLE) {
    vkDestroyFence(device, _inFlight, nullptr);
    _inFlight = VK_NULL_HANDLE;
  }
  _hasAcquiredImage = false;

  for (auto view : _imageViews) {
    vkDestroyImageView(device, view, nullptr);
  }
  _imageViews.clear();
  _images.clear();
  if (_swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, _swapchain, nullptr);
    _swapchain = VK_NULL_HANDLE;
  }
}

}  // namespace reng
