#include "vulkan_command_queue.h"

#include "reng/logger.h"
#include "vulkan_device.h"

namespace reng {

void VulkanCommandQueue::configure(VkQueue queue, uint32_t familyIndex) {
  _queue = queue;
  _familyIndex = familyIndex;
}

std::unique_ptr<QueueTimeline> VulkanCommandQueue::createTimeline(
    BackendDevice& device) {
  return std::make_unique<VulkanQueueTimeline>(device);
}

bool VulkanCommandQueue::initInner() {
  if (_queue == VK_NULL_HANDLE) {
    RengLogger::logError("Missing Vulkan queue for command queue");
    return false;
  }
  return true;
}

void VulkanCommandQueue::shutdownInner() { _queue = VK_NULL_HANDLE; }

}  // namespace reng
