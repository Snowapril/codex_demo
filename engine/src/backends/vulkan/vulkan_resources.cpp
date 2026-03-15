#include "vulkan_resources.h"

#include "reng/logger.h"

namespace reng {

void* VulkanResources::createBuffer(const ResourceId& id,
                                    const BufferCreateDesc& desc) {
  (void)id;
  (void)desc;
  RengLogger::logWarning("VulkanResources::createBuffer not implemented");
  return nullptr;
}

void* VulkanResources::createTexture(const ResourceId& id,
                                     const TextureCreateDesc& desc) {
  (void)id;
  (void)desc;
  RengLogger::logWarning("VulkanResources::createTexture not implemented");
  return nullptr;
}

void* VulkanResources::createSwapchainTexture(const ResourceId& id,
                                              const TextureCreateDesc& desc) {
  (void)id;
  (void)desc;
  RengLogger::logWarning(
      "VulkanResources::createSwapchainTexture not implemented");
  return nullptr;
}

void VulkanResources::destroyBuffer(void* handle) {
  (void)handle;
}

void VulkanResources::destroyTexture(void* handle) {
  (void)handle;
}

}  // namespace reng
