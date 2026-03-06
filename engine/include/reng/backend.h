#pragma once

#include <glm/vec2.hpp>

#include "reng/app.h"

namespace reng {

class BackendDevice {
 public:
  virtual ~BackendDevice() = default;
  virtual void shutdown() = 0;
};

class BackendSwapchain {
 public:
  virtual ~BackendSwapchain() = default;
  virtual bool recreate(const SwapchainDesc& desc) = 0;
  virtual void present() = 0;
  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;
  virtual PixelFormat colorFormat() const = 0;
  virtual ResourceId acquireNextImage() = 0;
};

class BackendResources {
 public:
  virtual ~BackendResources() = default;
  virtual void* createBuffer(const ResourceId& id,
                             const BufferCreateDesc& desc) = 0;
  virtual void* createTexture(const ResourceId& id,
                              const TextureCreateDesc& desc) = 0;
  virtual void* createSwapchainTexture(const ResourceId& id,
                                       const TextureCreateDesc& desc) = 0;
  virtual void destroyBuffer(void* handle) = 0;
  virtual void destroyTexture(void* handle) = 0;
};

class BackendCommand {
 public:
  virtual ~BackendCommand() = default;
};

}  // namespace reng
