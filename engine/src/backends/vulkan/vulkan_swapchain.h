#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "reng/app.h"
#include "reng/backend.h"
#include "vulkan_command_queue.h"
#include "vulkan_device.h"

namespace reng {

class VulkanSwapchain : public BackendSwapchain {
 public:
  VulkanSwapchain() = default;
  VulkanSwapchain(const VulkanSwapchain&) = delete;
  VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
  VulkanSwapchain(VulkanSwapchain&&) = delete;
  VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

  bool init(VulkanDevice& device, const SwapchainDesc& desc);
  bool init(VulkanDevice& device, VulkanCommandQueue* presentQueue,
            const SwapchainDesc& desc);
  bool recreate(const SwapchainDesc& desc) override;
  void shutdown(VkDevice device);
  void signalPresentReady() override {}
  void present() override;
  PixelFormat colorFormat() const override;
  ResourceId acquireNextImage() override;
  ResourceId swapchainResourceId() const override { return _swapchainResource; }

  VkSwapchainKHR swapchain() const { return _swapchain; }
  VkFormat format() const { return _format; }
  const std::vector<VkImageView>& imageViews() const { return _imageViews; }
  VkImageView currentImageView() const {
    if (_imageViews.empty()) {
      return VK_NULL_HANDLE;
    }
    return _imageViews[_acquiredImageIndex];
  }

 private:
  bool createSwapchainResources(const SwapchainDesc& desc);
  void destroySwapchainResources(VkDevice device);

  VulkanDevice* _device = nullptr;
  VulkanCommandQueue* _presentQueue = nullptr;
  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
  VkFormat _format = VK_FORMAT_B8G8R8A8_UNORM;
  VkExtent2D _extent{};
  std::vector<VkImage> _images;
  std::vector<VkImageView> _imageViews;
  VkFence _inFlight = VK_NULL_HANDLE;
  uint32_t _acquiredImageIndex = 0;
  bool _hasAcquiredImage = false;
  ResourceId _swapchainResource{1, ResourceKind::Texture, "swapchain_color"};
};

}  // namespace reng
