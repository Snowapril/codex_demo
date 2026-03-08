#pragma once

#import <Metal/Metal.h>
#import <Metal/MTL4Counters.h>

#include "metal_command_queue.h"
#include "metal_device.h"
#include "reng/command_buffer.h"
#include "reng/render_graph.h"

namespace reng {

class MetalCommandBuffer : public CommandBuffer {
 public:
  explicit MetalCommandBuffer(MetalDevice& device,
                              MetalCommandQueue& queue,
                              QueueType queueType);
  ~MetalCommandBuffer() override = default;

  CommandBufferTiming submit() override;

 protected:
  void onBeginCommandBuffer() override;
  void onEndCommandBuffer() override;
  void onBeginBlitPass() override;
  void onEndBlitPass() override;
  void onCopyTexture(const ResourceId& src, const ResourceId& dst) override;
  void onUploadBuffer(const ResourceId& buffer, size_t bytes) override;
  void onUploadTexture(const ResourceId& texture, size_t bytes) override;
  void onBeginRenderPass(const FramebufferDesc& framebuffer) override;
  void onEndRenderPass() override;
  void onDraw(uint32_t vertexCount, uint32_t instanceCount) override;
  void onBeginComputePass() override;
  void onEndComputePass() override;
  void onDispatch(uint32_t x, uint32_t y, uint32_t z) override;
  void onBeginMLPass() override;
  void onEndMLPass() override;
  void onDispatchML(uint32_t x, uint32_t y, uint32_t z) override;

 private:
  id<MTL4CommandBuffer> ensureCommandBuffer();
  id<MTLTexture> lookupTexture(const ResourceId& resource);

  MetalDevice& _device;
  MetalCommandQueue& _queue;
  QueueType _queueType = QueueType::Graphics;
  id<MTL4CommandAllocator> _allocator = nil;
  id<MTL4CommandBuffer> _commandBuffer = nil;
  id<MTL4CounterHeap> _timestampHeap = nil;
  uint64_t _timestampFrequency = 0;
  id<MTL4ComputeCommandEncoder> _blitEncoder = nil;
  id<MTL4RenderCommandEncoder> _renderEncoder = nil;
  id<MTL4ComputeCommandEncoder> _computeEncoder = nil;
  id<MTL4MachineLearningCommandEncoder> _mlEncoder = nil;
};

}  // namespace reng
