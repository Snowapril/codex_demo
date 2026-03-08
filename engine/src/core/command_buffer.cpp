#include "reng/command_buffer.h"

#include "reng/logger.h"
#include "reng/reng.h"

namespace reng {

CommandBuffer::CommandBuffer(BackendDevice& device, CommandQueue& queue)
    : _device(device), _queue(queue) {}

void CommandBuffer::setContext(ResourcePool* resourcePool,
                               BackendSwapchain* swapchain) {
  _resourcePool = resourcePool;
  _swapchain = swapchain;
}

void CommandBuffer::setTimelineValue(uint64_t value) {
  _timelineValue = value;
}

void CommandBuffer::beginCommandBuffer(bool enableTimestamps) {
  if (isRecording()) {
    RengLogger::logWarning("beginCommandBuffer called while recording");
    return;
  }
  setRecording(true);
  _timestampEnabled = enableTimestamps;
  onBeginCommandBuffer();
}

void CommandBuffer::endCommandBuffer() {
  if (!isRecording()) {
    RengLogger::logWarning("endCommandBuffer called without recording");
    return;
  }
  if (passState() != PassState::None) {
    RengLogger::logWarning("endCommandBuffer called with active pass");
    return;
  }
  onEndCommandBuffer();
  setRecording(false);
}

void CommandBuffer::beginBlitPass() {
  RENG_ASSERT(isRecording(),
              "beginBlitPass requires beginCommandBuffer to be called");
  if (passState() != PassState::None) {
    RengLogger::logWarning("beginBlitPass called while another pass is active");
    return;
  }
  setPassState(PassState::Blit);
  onBeginBlitPass();
}

void CommandBuffer::endBlitPass() {
  if (passState() != PassState::Blit) {
    RengLogger::logWarning("endBlitPass called without active blit pass");
    return;
  }
  onEndBlitPass();
  setPassState(PassState::None);
}

void CommandBuffer::copyTexture(const ResourceId& src,
                                const ResourceId& dst) {
  RENG_ASSERT(passState() == PassState::Blit,
              "copyTexture requires an active blit pass");
  onCopyTexture(src, dst);
}

void CommandBuffer::uploadBuffer(const ResourceId& buffer, size_t bytes) {
  RENG_ASSERT(passState() == PassState::Blit,
              "uploadBuffer requires an active blit pass");
  onUploadBuffer(buffer, bytes);
}

void CommandBuffer::uploadTexture(const ResourceId& texture, size_t bytes) {
  RENG_ASSERT(passState() == PassState::Blit,
              "uploadTexture requires an active blit pass");
  onUploadTexture(texture, bytes);
}

void CommandBuffer::beginRenderPass(const FramebufferDesc& framebuffer) {
  RENG_ASSERT(isRecording(),
              "beginRenderPass requires beginCommandBuffer to be called");
  if (passState() != PassState::None) {
    RengLogger::logWarning(
        "beginRenderPass called while another pass is active");
    return;
  }
  setPassState(PassState::Render);
  onBeginRenderPass(framebuffer);
}

void CommandBuffer::endRenderPass() {
  if (passState() != PassState::Render) {
    RengLogger::logWarning("endRenderPass called without active render pass");
    return;
  }
  onEndRenderPass();
  setPassState(PassState::None);
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount) {
  RENG_ASSERT(passState() == PassState::Render,
              "draw requires an active render pass");
  onDraw(vertexCount, instanceCount);
}

void CommandBuffer::beginComputePass() {
  RENG_ASSERT(isRecording(),
              "beginComputePass requires beginCommandBuffer to be called");
  if (passState() != PassState::None) {
    RengLogger::logWarning(
        "beginComputePass called while another pass is active");
    return;
  }
  setPassState(PassState::Compute);
  onBeginComputePass();
}

void CommandBuffer::endComputePass() {
  if (passState() != PassState::Compute) {
    RengLogger::logWarning(
        "endComputePass called without active compute pass");
    return;
  }
  onEndComputePass();
  setPassState(PassState::None);
}

void CommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z) {
  RENG_ASSERT(passState() == PassState::Compute,
              "dispatch requires an active compute pass");
  onDispatch(x, y, z);
}

void CommandBuffer::beginMLPass() {
  RENG_ASSERT(isRecording(),
              "beginMLPass requires beginCommandBuffer to be called");
  if (passState() != PassState::None) {
    RengLogger::logWarning("beginMLPass called while another pass is active");
    return;
  }
  setPassState(PassState::ML);
  onBeginMLPass();
}

void CommandBuffer::endMLPass() {
  if (passState() != PassState::ML) {
    RengLogger::logWarning("endMLPass called without active ML pass");
    return;
  }
  onEndMLPass();
  setPassState(PassState::None);
}

void CommandBuffer::dispatchML(uint32_t x, uint32_t y, uint32_t z) {
  RENG_ASSERT(passState() == PassState::ML,
              "dispatchML requires an active ML pass");
  onDispatchML(x, y, z);
}

}  // namespace reng
