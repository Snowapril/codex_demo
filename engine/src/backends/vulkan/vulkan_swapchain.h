#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "reng/app.h"
#include "reng/backend.h"
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
  bool recreate(const SwapchainDesc& desc) override;
  void shutdown(VkDevice device);
  void present() override;
  PixelFormat colorFormat() const override;

  VkSwapchainKHR swapchain() const { return _swapchain; }
  VkFormat format() const { return _format; }
  const std::vector<VkImageView>& imageViews() const { return _imageViews; }

 private:
  bool createSwapchainResources(const SwapchainDesc& desc);
 void destroySwapchainResources(VkDevice device);
  void recordCommandBuffer(uint32_t imageIndex);

  VulkanDevice* _device = nullptr;
  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
  VkFormat _format = VK_FORMAT_B8G8R8A8_UNORM;
  std::vector<VkImage> _images;
  std::vector<VkImageView> _imageViews;
  VkRenderPass _renderPass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> _framebuffers;
  VkCommandPool _commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> _commandBuffers;
  VkSemaphore _imageAvailable = VK_NULL_HANDLE;
  VkSemaphore _renderFinished = VK_NULL_HANDLE;
  VkFence _inFlight = VK_NULL_HANDLE;
};

}  // namespace reng
