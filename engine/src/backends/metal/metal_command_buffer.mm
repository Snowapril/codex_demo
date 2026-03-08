#include "metal_command_buffer.h"

#include "metal_swapchain.h"
#include "metal_utils.h"
#import <Metal/MTL4Counters.h>
#include "reng/backend.h"
#include "reng/logger.h"
#include "reng/reng.h"

namespace reng {

namespace {
constexpr size_t kMaxMetalColorAttachments = 8;

MTLLoadAction toMetalLoadAction(LoadAction action) {
  switch (action) {
    case LoadAction::Load:
      return MTLLoadActionLoad;
    case LoadAction::Clear:
      return MTLLoadActionClear;
    case LoadAction::DontCare:
    default:
      return MTLLoadActionDontCare;
  }
}

MTLStoreAction toMetalStoreAction(StoreAction action) {
  switch (action) {
    case StoreAction::Store:
      return MTLStoreActionStore;
    case StoreAction::DontCare:
    default:
      return MTLStoreActionDontCare;
  }
}
}  // namespace

MetalCommandBuffer::MetalCommandBuffer(BackendDevice& device,
                                       CommandQueue& queue)
    : CommandBuffer(device, queue) {}

CommandBufferTiming MetalCommandBuffer::submit() {
  CommandBufferTiming timing{};
  timing.queue = _queue.queueType();
  timing.timelineValue = timelineValue();
  RENG_ASSERT(!isRecording(),
              "submit requires endCommandBuffer to be called");
  if (!_commandBuffer) {
    return timing;
  }
  if (_blitEncoder) {
    [_blitEncoder endEncoding];
    _blitEncoder = nil;
  }
  if (_renderEncoder) {
    [_renderEncoder endEncoding];
    _renderEncoder = nil;
  }
  if (_computeEncoder) {
    [_computeEncoder endEncoding];
    _computeEncoder = nil;
  }
  if (_mlEncoder) {
    [_mlEncoder endEncoding];
    _mlEncoder = nil;
  }

  MetalCommandQueue& metalQueue = static_cast<MetalCommandQueue&>(queue());
  id<MTL4CommandBuffer> buffers[] = {_commandBuffer};
  [metalQueue.queue() commit:buffers count:1];
  if (auto* timeline = metalQueue.metalTimeline()) {
    timeline->signalQueue(metalQueue.queue(), timelineValue());
  }

  _commandBuffer = nil;
  _timestampHeap = nil;
  return timing;
}

id<MTL4CommandBuffer> MetalCommandBuffer::ensureCommandBuffer() {
  if (_commandBuffer) {
    return _commandBuffer;
  }
  MetalCommandQueue& metalQueue = static_cast<MetalCommandQueue&>(queue());
  if (!metalQueue.queue()) {
    RengLogger::logError("Metal command buffer missing command queue");
    return nil;
  }
  auto& metalDevice = static_cast<MetalDevice&>(device());
  id<MTLDevice> nativeDevice = metalDevice.device();
  if (!nativeDevice) {
    RengLogger::logError("Metal device missing for command buffer");
    return nil;
  }
  if (!_allocator) {
    _allocator = [nativeDevice newCommandAllocator];
    if (!_allocator) {
      RengLogger::logError("Failed to create Metal command allocator");
      return nil;
    }
  }
  _commandBuffer = [nativeDevice newCommandBuffer];
  if (!_commandBuffer) {
    RengLogger::logError("Failed to create Metal command buffer");
  }
  return _commandBuffer;
}

void MetalCommandBuffer::onBeginCommandBuffer() {
  id<MTL4CommandBuffer> commandBuffer = ensureCommandBuffer();
  if (!commandBuffer) {
    return;
  }
  [commandBuffer beginCommandBufferWithAllocator:_allocator];
  if (timestampEnabled()) {
    MetalCommandQueue& metalQueue = static_cast<MetalCommandQueue&>(queue());
    _timestampHeap = metalQueue.acquireTimestampHeap(timelineValue());
    if (_timestampFrequency == 0) {
      auto& metalDevice = static_cast<MetalDevice&>(device());
      if (metalDevice.device()) {
        _timestampFrequency = [metalDevice.device() queryTimestampFrequency];
      }
    }
    if (_timestampHeap) {
      [_timestampHeap invalidateCounterRange:NSMakeRange(0, 2)];
      [commandBuffer writeTimestampIntoHeap:_timestampHeap atIndex:0];
    }
  }
}

void MetalCommandBuffer::onEndCommandBuffer() {
  if (_commandBuffer) {
    if (timestampEnabled() && _timestampHeap) {
      [_commandBuffer writeTimestampIntoHeap:_timestampHeap atIndex:1];
    }
    [_commandBuffer endCommandBuffer];
  }
}

id<MTLTexture> MetalCommandBuffer::lookupTexture(const ResourceId& resource) {
  auto* swap = swapchain();
  if (swap && resource == swap->swapchainResourceId()) {
    auto* metalSwap = static_cast<MetalSwapchain*>(swap);
    return metalSwap->currentTexture();
  }
  if (!resourcePool()) {
    return nil;
  }
  return (__bridge id<MTLTexture>)resourcePool()->lookupTexture(resource);
}

void MetalCommandBuffer::onBeginBlitPass() {
  id<MTL4CommandBuffer> commandBuffer = ensureCommandBuffer();
  if (!commandBuffer) {
    return;
  }
  _blitEncoder = [commandBuffer computeCommandEncoder];
}

void MetalCommandBuffer::onEndBlitPass() {
  if (_blitEncoder) {
    [_blitEncoder endEncoding];
    _blitEncoder = nil;
  }
}

void MetalCommandBuffer::onCopyTexture(const ResourceId& src,
                                       const ResourceId& dst) {
  if (!_blitEncoder) {
    RengLogger::logWarning("copyTexture called without active blit encoder");
    return;
  }
  id<MTLTexture> srcTexture = lookupTexture(src);
  id<MTLTexture> dstTexture = lookupTexture(dst);
  if (!srcTexture || !dstTexture) {
    RengLogger::logWarning("copyTexture missing source/destination texture");
    return;
  }
  [_blitEncoder copyFromTexture:srcTexture toTexture:dstTexture];
}

void MetalCommandBuffer::onUploadBuffer(const ResourceId& buffer, size_t bytes) {
  (void)buffer;
  (void)bytes;
  RengLogger::logWarning("uploadBuffer not implemented for Metal backend");
}

void MetalCommandBuffer::onUploadTexture(const ResourceId& texture,
                                         size_t bytes) {
  (void)texture;
  (void)bytes;
  RengLogger::logWarning("uploadTexture not implemented for Metal backend");
}

void MetalCommandBuffer::onBeginRenderPass(const FramebufferDesc& framebuffer) {
  id<MTL4CommandBuffer> commandBuffer = ensureCommandBuffer();
  if (!commandBuffer) {
    return;
  }

  MTL4RenderPassDescriptor* descriptor =
      [[MTL4RenderPassDescriptor alloc] init];
  if (!descriptor) {
    return;
  }

  for (size_t i = 0; i < framebuffer.colorAttachments.size(); ++i) {
    if (i >= kMaxMetalColorAttachments) {
      RengLogger::logWarning("Exceeded Metal color attachment limit");
      break;
    }
    const auto& attachment = framebuffer.colorAttachments[i];
    id<MTLTexture> texture = lookupTexture(attachment.resource);
    if (!texture) {
      RengLogger::logWarning(
          "Render pass missing color attachment texture {}", attachment.resource.name);
      continue;
    }
    auto* color = descriptor.colorAttachments[i];
    color.texture = texture;
    color.loadAction = toMetalLoadAction(attachment.loadAction);
    color.storeAction = toMetalStoreAction(attachment.storeAction);
    color.clearColor = MTLClearColorMake(attachment.clearColor.r,
                                         attachment.clearColor.g,
                                         attachment.clearColor.b,
                                         attachment.clearColor.a);
  }

  if (framebuffer.depthStencilAttachment.has_value()) {
    const auto& attachment = framebuffer.depthStencilAttachment.value();
    id<MTLTexture> texture = lookupTexture(attachment.resource);
    if (!texture) {
      RengLogger::logWarning(
          "Render pass missing depth/stencil texture {}", attachment.resource.name);
    } else {
      descriptor.depthAttachment.texture = texture;
      descriptor.depthAttachment.loadAction =
          toMetalLoadAction(attachment.loadAction);
      descriptor.depthAttachment.storeAction =
          toMetalStoreAction(attachment.storeAction);
      descriptor.depthAttachment.clearDepth = attachment.clearDepth;
      descriptor.stencilAttachment.texture = texture;
      descriptor.stencilAttachment.loadAction =
          toMetalLoadAction(attachment.loadAction);
      descriptor.stencilAttachment.storeAction =
          toMetalStoreAction(attachment.storeAction);
      descriptor.stencilAttachment.clearStencil = attachment.clearStencil;
    }
  }

  _renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
}

void MetalCommandBuffer::onEndRenderPass() {
  if (_renderEncoder) {
    [_renderEncoder endEncoding];
    _renderEncoder = nil;
  }
}

void MetalCommandBuffer::onDraw(uint32_t vertexCount, uint32_t instanceCount) {
  (void)instanceCount;
  if (vertexCount == 0) {
    return;
  }
  RengLogger::logWarning("Draw skipped: Metal pipeline binding not implemented");
}

void MetalCommandBuffer::onBeginComputePass() {
  id<MTL4CommandBuffer> commandBuffer = ensureCommandBuffer();
  if (!commandBuffer) {
    return;
  }
  _computeEncoder = [commandBuffer computeCommandEncoder];
}

void MetalCommandBuffer::onEndComputePass() {
  if (_computeEncoder) {
    [_computeEncoder endEncoding];
    _computeEncoder = nil;
  }
}

void MetalCommandBuffer::onDispatch(uint32_t x, uint32_t y, uint32_t z) {
  (void)x;
  (void)y;
  (void)z;
  if (!_computeEncoder) {
    RengLogger::logWarning("dispatch called without active compute encoder");
    return;
  }
  RengLogger::logWarning(
      "Dispatch skipped: Metal compute pipeline binding not implemented");
}

void MetalCommandBuffer::onBeginMLPass() {
  id<MTL4CommandBuffer> commandBuffer = ensureCommandBuffer();
  if (!commandBuffer) {
    return;
  }
  _mlEncoder = [commandBuffer machineLearningCommandEncoder];
}

void MetalCommandBuffer::onEndMLPass() {
  if (_mlEncoder) {
    [_mlEncoder endEncoding];
    _mlEncoder = nil;
  }
}

void MetalCommandBuffer::onDispatchML(uint32_t x, uint32_t y, uint32_t z) {
  (void)x;
  (void)y;
  (void)z;
  if (!_mlEncoder) {
    RengLogger::logWarning("dispatchML called without active ML encoder");
    return;
  }
  RengLogger::logWarning("ML dispatch not implemented for Metal backend");
}

}  // namespace reng
