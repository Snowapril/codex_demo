#pragma once

#include <vulkan/vulkan.h>

#include "reng/command_queue.h"
#include "vulkan_queue_timeline.h"

namespace reng {

class VulkanDevice;

class VulkanCommandQueue : public CommandQueue {
 public:
  void configure(VkQueue queue, uint32_t familyIndex);
  void shutdown() { CommandQueue::shutdown(); }

  VkQueue queue() const { return _queue; }
  uint32_t familyIndex() const { return _familyIndex; }
  VulkanQueueTimeline* vulkanTimeline() {
    return static_cast<VulkanQueueTimeline*>(timeline());
  }

 private:
  std::unique_ptr<QueueTimeline> createTimeline(
      BackendDevice& device) override;
  bool initInner() override;
  void shutdownInner() override;

  VkQueue _queue = VK_NULL_HANDLE;
  uint32_t _familyIndex = 0;
};

}  // namespace reng
