#pragma once

#include <unordered_map>

#include "reng/resources.h"
#include "reng/backend.h"

namespace reng {

class ResourcePoolImpl final : public ResourcePool {
 public:
  explicit ResourcePoolImpl(BackendResources& backend);
  ~ResourcePoolImpl() override;

  bool addBuffer(const ResourceId& id, const BufferCreateDesc& desc) override;
  bool addTexture(const ResourceId& id, const TextureCreateDesc& desc) override;
  bool addSwapchainTexture(const ResourceId& id,
                           const TextureCreateDesc& desc) override;
  void* lookupBuffer(const ResourceId& id) override;
  void* lookupTexture(const ResourceId& id) override;

 private:
  struct ResourceKey {
    uint32_t value = 0;
    ResourceKind kind = ResourceKind::Buffer;

    bool operator==(const ResourceKey& other) const {
      return value == other.value && kind == other.kind;
    }
  };

  struct ResourceKeyHash {
    size_t operator()(const ResourceKey& key) const {
      return (static_cast<size_t>(key.value) << 1) ^
             static_cast<size_t>(key.kind);
    }
  };

  struct ResourceEntry {
    ResourceId id;
    void* handle = nullptr;
    bool swapchain = false;
  };

  bool addResource(const ResourceId& id, void* handle, bool swapchain);
  void* lookupResource(const ResourceId& id);

  BackendResources& _backend;
  std::unordered_map<ResourceKey, ResourceEntry, ResourceKeyHash> _resources;
};

}  // namespace reng
