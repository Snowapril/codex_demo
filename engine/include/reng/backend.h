#pragma once

#include <cstddef>
#include <glm/vec2.hpp>

#include <memory>

#include "reng/app.h"
#include "reng/command_buffer.h"
#include "reng/command_queue.h"
#include "reng/reng.h"

namespace reng {

class BackendDevice {
 public:
  virtual ~BackendDevice() = default;
  virtual void shutdown() = 0;
  virtual CommandQueue* graphicsQueue() const = 0;
  virtual CommandQueue* computeQueue() const = 0;
  virtual size_t copyQueueCount() const = 0;
  virtual CommandQueue* copyQueue(size_t index) const = 0;
};

class BackendSwapchain {
 public:
  virtual ~BackendSwapchain() = default;
  virtual bool recreate(const SwapchainDesc& desc) = 0;
  virtual void signalPresentReady() = 0;
  virtual void present() = 0;
  virtual PixelFormat colorFormat() const = 0;
  virtual reng_nodiscard ResourceId acquireNextImage() = 0;
  virtual ResourceId swapchainResourceId() const = 0;
  virtual void setCurrentDrawable(void* drawable) = 0;

  reng_inline uint32_t width() const { return _size.x; }
  reng_inline uint32_t height() const { return _size.y; }

 protected:
  glm::uvec2 _size{0, 0};
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
