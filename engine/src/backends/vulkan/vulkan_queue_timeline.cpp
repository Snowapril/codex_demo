#include "vulkan_queue_timeline.h"

#include "vulkan_utils.h"

namespace reng {

bool VulkanQueueTimeline::init(VkDevice device) {
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

  _value = 0;
  _signalValue = 0;
  return true;
}

void VulkanQueueTimeline::shutdown(VkDevice device) {
  if (_semaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(device, _semaphore, nullptr);
    _semaphore = VK_NULL_HANDLE;
  }
}

uint64_t VulkanQueueTimeline::prepareSignal(
    VkSubmitInfo& submitInfo,
    VkTimelineSemaphoreSubmitInfo& timelineInfo) {
  _signalValue = nextValue();

  timelineInfo = {VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
  timelineInfo.signalSemaphoreValueCount = 1;
  timelineInfo.pSignalSemaphoreValues = &_signalValue;

  submitInfo.pNext = &timelineInfo;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &_semaphore;

  return _signalValue;
}

}  // namespace reng
