#include "vulkan_queue_timeline.h"

#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace reng {

bool VulkanQueueTimeline::initInner() {
  VkDevice device = static_cast<VulkanDevice&>(_device).device();

  VkSemaphoreTypeCreateInfo typeInfo{
      VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
  typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  typeInfo.initialValue = 0;

  VkSemaphoreCreateInfo createInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  createInfo.pNext = &typeInfo;

  if (!vulkan::check(
          vkCreateSemaphore(device, &createInfo, nullptr, &_semaphore),
          "vkCreateSemaphore (timeline) failed")) {
    return false;
  }
  return true;
}

uint64_t VulkanQueueTimeline::completedValue() const {
  VkDevice device = static_cast<const VulkanDevice&>(_device).device();
  if (_semaphore == VK_NULL_HANDLE) {
    return 0;
  }
  uint64_t value = 0;
  VkResult result =
      vkGetSemaphoreCounterValue(device, _semaphore, &value);
  return result == VK_SUCCESS ? value : 0;
}

void VulkanQueueTimeline::shutdownInner() {
  VkDevice device = static_cast<VulkanDevice&>(_device).device();
  if (_semaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(device, _semaphore, nullptr);
    _semaphore = VK_NULL_HANDLE;
  }
}

}  // namespace reng
