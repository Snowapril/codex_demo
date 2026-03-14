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
  auto& device = static_cast<VulkanDevice&>(this->device());
  if (device.timestampValidBits() == 0 || device.timestampPeriod() <= 0.0) {
    return VK_NULL_HANDLE;
  }
  for (auto& entry : _timestampPools) {
    if (entry.pool != VK_NULL_HANDLE && !entry.pending &&
        completed >= entry.lastValue) {
      entry.lastValue = timelineValue;
      entry.pending = true;
      _timestampPoolByValue[timelineValue] =
          static_cast<size_t>(&entry - _timestampPools.data());
      return entry.pool;
    }
  }

  VkQueryPoolCreateInfo queryInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
  queryInfo.queryCount = 2;
  VkQueryPool pool = VK_NULL_HANDLE;
  VkDevice vkDevice = device.device();
  if (!vulkan::check(
          vkCreateQueryPool(vkDevice, &queryInfo, nullptr, &pool),
          "vkCreateQueryPool failed")) {
    return VK_NULL_HANDLE;
  }
  _timestampPools.push_back({pool, timelineValue, true});
  _timestampPoolByValue[timelineValue] = _timestampPools.size() - 1;
  return pool;
}

bool VulkanCommandQueue::resolveTimestamp(uint64_t timelineValue,
                                          CommandBufferTiming& timing) {
  auto it = _timestampPoolByValue.find(timelineValue);
  if (it == _timestampPoolByValue.end()) {
    return false;
  }
  if (it->second >= _timestampPools.size()) {
    return false;
  }
  auto& entry = _timestampPools[it->second];
  if (entry.pool == VK_NULL_HANDLE || !entry.pending) {
    return false;
  }
  auto* timeline = vulkanTimeline();
  uint64_t completed = timeline ? timeline->completedValue() : 0;
  if (completed < timelineValue) {
    return false;
  }

  auto& device = static_cast<VulkanDevice&>(this->device());
  uint64_t timestamps[2] = {};
  VkResult result = vkGetQueryPoolResults(
      device.device(), entry.pool, 0, 2, sizeof(timestamps), timestamps,
      sizeof(uint64_t),
      VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
  if (result != VK_SUCCESS) {
    RengLogger::logWarning(
        "Vulkan timestamp resolve failed for timeline {} (VkResult {})",
        timelineValue, static_cast<int>(result));
    return false;
  }
  double periodNs = device.timestampPeriod();
  if (periodNs <= 0.0) {
    RengLogger::logWarning("Vulkan timestamp period invalid");
    return false;
  }
  timing.gpuStartNs = static_cast<uint64_t>(timestamps[0] * periodNs);
  timing.gpuEndNs = static_cast<uint64_t>(timestamps[1] * periodNs);
  timing.valid = true;
  entry.pending = false;
  _timestampPoolByValue.erase(it);
  return true;
}

void VulkanCommandQueue::shutdownInner() {
  VkDevice device = static_cast<VulkanDevice&>(this->device()).device();
  for (auto& entry : _timestampPools) {
    if (entry.pool != VK_NULL_HANDLE) {
      vkDestroyQueryPool(device, entry.pool, nullptr);
    }
  }
  _timestampPools.clear();
  _timestampPoolByValue.clear();
  _queue = VK_NULL_HANDLE;
}

}  // namespace reng
