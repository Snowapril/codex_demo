#pragma once

#include "reng/backend.h"

namespace reng {

class VulkanResources : public BackendResources {
 public:
  void* createBuffer(const ResourceId& id,
                     const BufferCreateDesc& desc) override;
  void* createTexture(const ResourceId& id,
                      const TextureCreateDesc& desc) override;
  void* createSwapchainTexture(const ResourceId& id,
                               const TextureCreateDesc& desc) override;
  void destroyBuffer(void* handle) override;
  void destroyTexture(void* handle) override;
};

}  // namespace reng
