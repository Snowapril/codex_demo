#pragma once

#include <unordered_map>
#include <vector>

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
  VkQueryPool acquireTimestampPool(uint64_t timelineValue);
  bool resolveTimestamp(uint64_t timelineValue,
                        CommandBufferTiming& timing) override;

 private:
  std::unique_ptr<QueueTimeline> createTimeline(
      BackendDevice& device) override;
  std::unique_ptr<CommandBufferPool> createCommandBufferPool(
      BackendDevice& device) override;
  bool initInner() override;
  void shutdownInner() override;

  VkQueue _queue = VK_NULL_HANDLE;
  uint32_t _familyIndex = 0;
  struct TimestampPoolEntry {
    VkQueryPool pool = VK_NULL_HANDLE;
    uint64_t lastValue = 0;
    bool pending = false;
  };
  std::vector<TimestampPoolEntry> _timestampPools;
  std::unordered_map<uint64_t, size_t> _timestampPoolByValue;
};

}  // namespace reng
