#pragma once

#include "metal_device.h"
#include "reng/command_buffer.h"
#include "reng/render_graph.h"

namespace reng {

class MetalCommandBuffer : public CommandBuffer {
 public:
  explicit MetalCommandBuffer(MetalDevice& device, QueueType queueType)
      : _device(device), _queueType(queueType) {}

 protected:
  void onBeginBlitPass() override {}
  void onEndBlitPass() override {}
  void onCopyTexture(const ResourceId& src, const ResourceId& dst) override {
    (void)src;
    (void)dst;
  }
  void onUploadBuffer(const ResourceId& buffer, size_t bytes) override {
    (void)buffer;
    (void)bytes;
  }
  void onUploadTexture(const ResourceId& texture, size_t bytes) override {
    (void)texture;
    (void)bytes;
  }
  void onBeginRenderPass(const FramebufferDesc& framebuffer) override {
    (void)framebuffer;
  }
  void onEndRenderPass() override {}
  void onDraw(uint32_t vertexCount, uint32_t instanceCount) override {
    (void)vertexCount;
    (void)instanceCount;
  }
  void onBeginComputePass() override {}
  void onEndComputePass() override {}
  void onDispatch(uint32_t x, uint32_t y, uint32_t z) override {
    (void)x;
    (void)y;
    (void)z;
  }
  void onBeginMLPass() override {}
  void onEndMLPass() override {}
  void onDispatchML(uint32_t x, uint32_t y, uint32_t z) override {
    (void)x;
    (void)y;
    (void)z;
  }

 private:
  MetalDevice& _device;
  QueueType _queueType = QueueType::Graphics;
};

}  // namespace reng
