#pragma once

#include <cstddef>

#include "reng/resources.h"

namespace reng {

class CommandBuffer {
 public:
  virtual ~CommandBuffer() = default;

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

 private:
  PassState _passState = PassState::None;
};

}  // namespace reng
