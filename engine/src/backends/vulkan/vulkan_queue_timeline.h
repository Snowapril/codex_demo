#pragma once

#include <vulkan/vulkan.h>

#include "reng/queue_timeline.h"

namespace reng {

class VulkanQueueTimeline : public QueueTimeline {
 public:
  bool init(VkDevice device);
  void shutdown(VkDevice device);

  VkSemaphore semaphore() const { return _semaphore; }

  uint64_t prepareSignal(VkSubmitInfo& submitInfo,
                         VkTimelineSemaphoreSubmitInfo& timelineInfo);

 private:
  VkSemaphore _semaphore = VK_NULL_HANDLE;
  uint64_t _signalValue = 0;
};

}  // namespace reng
