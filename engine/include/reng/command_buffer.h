#pragma once

#include <cstddef>
#include <cstdint>

#include "reng/queue_types.h"
#include "reng/resources.h"

namespace reng {

class BackendSwapchain;
class ResourcePool;

struct CommandBufferTiming {
  QueueType queue = QueueType::Graphics;
  uint64_t timelineValue = 0;
  uint64_t gpuStartNs = 0;
  uint64_t gpuEndNs = 0;
  bool valid = false;
};

class CommandBuffer {
 public:
  virtual ~CommandBuffer() = default;

  void setContext(ResourcePool* resourcePool, BackendSwapchain* swapchain);
  void setTimelineValue(uint64_t value);
  void beginCommandBuffer();
  void endCommandBuffer();
  virtual CommandBufferTiming submit() = 0;
  bool isRecording() const { return _recording; }

  void beginBlitPass();
  void endBlitPass();
  void copyTexture(const ResourceId& src, const ResourceId& dst);
  void uploadBuffer(const ResourceId& buffer, size_t bytes);
  void uploadTexture(const ResourceId& texture, size_t bytes);

  void beginRenderPass(const FramebufferDesc& framebuffer);
  void endRenderPass();
  void draw(uint32_t vertexCount, uint32_t instanceCount);

  void beginComputePass();
  void endComputePass();
  void dispatch(uint32_t x, uint32_t y, uint32_t z);

  void beginMLPass();
  void endMLPass();
  void dispatchML(uint32_t x, uint32_t y, uint32_t z);

 protected:
  enum class PassState : uint8_t {
    None,
    Blit,
    Render,
    Compute,
    ML,
  };

  virtual void onBeginCommandBuffer() = 0;
  virtual void onEndCommandBuffer() = 0;
  virtual void onBeginBlitPass() = 0;
  virtual void onEndBlitPass() = 0;
  virtual void onCopyTexture(const ResourceId& src, const ResourceId& dst) = 0;
  virtual void onUploadBuffer(const ResourceId& buffer, size_t bytes) = 0;
  virtual void onUploadTexture(const ResourceId& texture, size_t bytes) = 0;

  virtual void onBeginRenderPass(const FramebufferDesc& framebuffer) = 0;
  virtual void onEndRenderPass() = 0;
  virtual void onDraw(uint32_t vertexCount, uint32_t instanceCount) = 0;

  virtual void onBeginComputePass() = 0;
  virtual void onEndComputePass() = 0;
  virtual void onDispatch(uint32_t x, uint32_t y, uint32_t z) = 0;

  virtual void onBeginMLPass() = 0;
  virtual void onEndMLPass() = 0;
  virtual void onDispatchML(uint32_t x, uint32_t y, uint32_t z) = 0;

  PassState passState() const { return _passState; }
  void setPassState(PassState state) { _passState = state; }
  ResourcePool* resourcePool() const { return _resourcePool; }
  BackendSwapchain* swapchain() const { return _swapchain; }
  uint64_t timelineValue() const { return _timelineValue; }
  void setRecording(bool recording) { _recording = recording; }

 private:
  PassState _passState = PassState::None;
  ResourcePool* _resourcePool = nullptr;
  BackendSwapchain* _swapchain = nullptr;
  uint64_t _timelineValue = 0;
  bool _recording = false;
};

}  // namespace reng
