#include "vulkan_command_queue.h"

#include "reng/logger.h"
#include "vulkan_command_buffer_pool.h"
#include "vulkan_device.h"
#include "vulkan_utils.h"

namespace reng {

void VulkanCommandQueue::configure(VkQueue queue, uint32_t familyIndex) {
  _queue = queue;
  _familyIndex = familyIndex;
}

std::unique_ptr<QueueTimeline> VulkanCommandQueue::createTimeline(
    BackendDevice& device) {
  return std::make_unique<VulkanQueueTimeline>(device);
}

std::unique_ptr<CommandBufferPool> VulkanCommandQueue::createCommandBufferPool(
    BackendDevice& device) {
  return std::make_unique<VulkanCommandBufferPool>(
      *this, static_cast<VulkanDevice&>(device));
}

bool VulkanCommandQueue::initInner() {
  if (_queue == VK_NULL_HANDLE) {
    RengLogger::logError("Missing Vulkan queue for command queue");
    return false;
  }
  return true;
}

VkQueryPool VulkanCommandQueue::acquireTimestampPool(uint64_t timelineValue) {
  auto* timeline = vulkanTimeline();
  uint64_t completed = timeline ? timeline->completedValue() : 0;
  for (auto& entry : _timestampPools) {
    if (entry.pool != VK_NULL_HANDLE && completed >= entry.lastValue) {
      entry.lastValue = timelineValue;
      return entry.pool;
    }
  }

  VkQueryPoolCreateInfo queryInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
  queryInfo.queryCount = 2;
  VkQueryPool pool = VK_NULL_HANDLE;
  VkDevice device = static_cast<VulkanDevice&>(this->device()).device();
  if (!vulkan::check(
          vkCreateQueryPool(device, &queryInfo, nullptr, &pool),
          "vkCreateQueryPool failed")) {
    return VK_NULL_HANDLE;
  }
  _timestampPools.push_back({pool, timelineValue});
  return pool;
}

void VulkanCommandQueue::shutdownInner() {
  VkDevice device = static_cast<VulkanDevice&>(this->device()).device();
  for (auto& entry : _timestampPools) {
    if (entry.pool != VK_NULL_HANDLE) {
      vkDestroyQueryPool(device, entry.pool, nullptr);
    }
  }
  _timestampPools.clear();
  _queue = VK_NULL_HANDLE;
}

}  // namespace reng
