#pragma once

#include <vulkan/vulkan.h>

#include "reng/queue_timeline.h"

namespace reng {

class VulkanQueueTimeline : public QueueTimeline {
 public:
  VulkanQueueTimeline(BackendDevice& device) : QueueTimeline(device) {}
  ~VulkanQueueTimeline() override = default;

  VkSemaphore semaphore() const { return _semaphore; }

 protected:
  bool initInner() override final;
  void shutdownInner() override final;

 private:
  VkSemaphore _semaphore = VK_NULL_HANDLE;
};

}  // namespace reng
