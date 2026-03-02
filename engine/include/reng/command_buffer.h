#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace reng {

enum class CommandType : uint8_t {
  BeginBlitPass,
  EndBlitPass,
  CopyTexture,
  UploadBuffer,
  UploadTexture,
  BeginRenderPass,
  EndRenderPass,
  Draw,
  BeginComputePass,
  EndComputePass,
  Dispatch,
  BeginMLPass,
  EndMLPass,
};

struct Command {
  CommandType type;
  std::string a;
  std::string b;
  uint64_t x = 0;
  uint64_t y = 0;
  uint64_t z = 0;
};

class CommandBuffer {
 public:
  void beginBlitPass();
  void endBlitPass();
  void copyTexture(const std::string& src, const std::string& dst);
  void uploadBuffer(const std::string& name, size_t bytes);
  void uploadTexture(const std::string& name, size_t bytes);

  void beginRenderPass(const std::string& framebufferName);
  void endRenderPass();
  void draw(uint32_t vertexCount, uint32_t instanceCount);

  void beginComputePass();
  void endComputePass();
  void dispatch(uint32_t x, uint32_t y, uint32_t z);

  void beginMLPass();
  void endMLPass();
  void dispatchML(uint32_t x, uint32_t y, uint32_t z);

  const std::vector<Command>& commands() const { return _commands; }

 private:
  std::vector<Command> _commands;
};

}  // namespace reng
