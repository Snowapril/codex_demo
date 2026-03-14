#include "core/resource_pool.h"

#include "reng/logger.h"

namespace reng {

ResourcePoolImpl::ResourcePoolImpl(BackendResources& backend)
    : _backend(backend) {}

ResourcePoolImpl::~ResourcePoolImpl() {
  for (const auto& [key, entry] : _resources) {
    if (entry.id.kind == ResourceKind::Buffer) {
      _backend.destroyBuffer(entry.handle);
    } else {
      _backend.destroyTexture(entry.handle);
    }
  }
  _resources.clear();
}

bool ResourcePoolImpl::addResource(const ResourceId& id, void* handle,
                                   bool swapchain) {
  ResourceKey key{id.value, id.kind};
  if (_resources.find(key) != _resources.end()) {
    RengLogger::logWarning("Resource already exists: {}", id.name);
    return false;
  }
  ResourceEntry entry;
  entry.id = id;
  entry.handle = handle;
  entry.swapchain = swapchain;
  _resources.emplace(key, entry);
  return true;
}

bool ResourcePoolImpl::addBuffer(const ResourceId& id,
                                 const BufferCreateDesc& desc) {
  if (id.kind != ResourceKind::Buffer) {
    RengLogger::logWarning("addBuffer called with non-buffer id {}", id.name);
    return false;
  }
  void* handle = _backend.createBuffer(id, desc);
  return addResource(id, handle, false);
}

bool ResourcePoolImpl::addTexture(const ResourceId& id,
                                  const TextureCreateDesc& desc) {
  if (id.kind != ResourceKind::Texture) {
    RengLogger::logWarning("addTexture called with non-texture id {}", id.name);
    return false;
  }
  void* handle = _backend.createTexture(id, desc);
  return addResource(id, handle, false);
}

bool ResourcePoolImpl::addSwapchainTexture(const ResourceId& id,
                                           const TextureCreateDesc& desc) {
  if (id.kind != ResourceKind::Texture) {
    RengLogger::logWarning("addSwapchainTexture called with non-texture id {}",
                           id.name);
    return false;
  }
  void* handle = _backend.createSwapchainTexture(id, desc);
  return addResource(id, handle, true);
}

void* ResourcePoolImpl::lookupResource(const ResourceId& id) {
  ResourceKey key{id.value, id.kind};
  auto it = _resources.find(key);
  if (it == _resources.end()) {
    return nullptr;
  }
  return it->second.handle;
}

void* ResourcePoolImpl::lookupBuffer(const ResourceId& id) {
  if (id.kind != ResourceKind::Buffer) {
    return nullptr;
  }
  return lookupResource(id);
}

void* ResourcePoolImpl::lookupTexture(const ResourceId& id) {
  if (id.kind != ResourceKind::Texture) {
    return nullptr;
  }
  return lookupResource(id);
}

}  // namespace reng
