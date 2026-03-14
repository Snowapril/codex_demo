#include "metal_resources.h"

#include "reng/logger.h"

namespace reng {

void* MetalResources::createBuffer(const ResourceId& id,
                                   const BufferCreateDesc& desc) {
  (void)id;
  (void)desc;
  RengLogger::logWarning("MetalResources::createBuffer not implemented");
  return nullptr;
}

void* MetalResources::createTexture(const ResourceId& id,
                                    const TextureCreateDesc& desc) {
  (void)id;
  (void)desc;
  RengLogger::logWarning("MetalResources::createTexture not implemented");
  return nullptr;
}

void* MetalResources::createSwapchainTexture(const ResourceId& id,
                                             const TextureCreateDesc& desc) {
  (void)id;
  (void)desc;
  RengLogger::logWarning(
      "MetalResources::createSwapchainTexture not implemented");
  return nullptr;
}

void MetalResources::destroyBuffer(void* handle) {
  (void)handle;
}

void MetalResources::destroyTexture(void* handle) {
  (void)handle;
}

}  // namespace reng
