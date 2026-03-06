#include "reng/command_buffer.h"

namespace reng {

void CommandBuffer::beginBlitPass() {
  _commands.push_back({CommandType::BeginBlitPass});
}

void CommandBuffer::endBlitPass() {
  _commands.push_back({CommandType::EndBlitPass});
}

void CommandBuffer::copyTexture(const ResourceId& src,
                                const ResourceId& dst) {
  Command cmd{CommandType::CopyTexture};
  cmd.a = src;
  cmd.b = dst;
  _commands.push_back(cmd);
}

void CommandBuffer::uploadBuffer(const ResourceId& buffer, size_t bytes) {
  Command cmd{CommandType::UploadBuffer};
  cmd.a = buffer;
  cmd.x = bytes;
  _commands.push_back(cmd);
}

void CommandBuffer::uploadTexture(const ResourceId& texture, size_t bytes) {
  Command cmd{CommandType::UploadTexture};
  cmd.a = texture;
  cmd.x = bytes;
  _commands.push_back(cmd);
}

void CommandBuffer::beginRenderPass(const FramebufferDesc& framebuffer) {
  Command cmd{CommandType::BeginRenderPass};
  cmd.framebuffer = framebuffer;
  _commands.push_back(cmd);
}

void CommandBuffer::endRenderPass() {
  _commands.push_back({CommandType::EndRenderPass});
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount) {
  Command cmd{CommandType::Draw};
  cmd.x = vertexCount;
  cmd.y = instanceCount;
  _commands.push_back(cmd);
}

void CommandBuffer::beginComputePass() {
  _commands.push_back({CommandType::BeginComputePass});
}

void CommandBuffer::endComputePass() {
  _commands.push_back({CommandType::EndComputePass});
}

void CommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z) {
  Command cmd{CommandType::Dispatch};
  cmd.x = x;
  cmd.y = y;
  cmd.z = z;
  _commands.push_back(cmd);
}

void CommandBuffer::beginMLPass() {
  _commands.push_back({CommandType::BeginMLPass});
}

void CommandBuffer::endMLPass() {
  _commands.push_back({CommandType::EndMLPass});
}

void CommandBuffer::dispatchML(uint32_t x, uint32_t y, uint32_t z) {
  Command cmd{CommandType::Dispatch};
  cmd.x = x;
  cmd.y = y;
  cmd.z = z;
  _commands.push_back(cmd);
}

}  // namespace reng
