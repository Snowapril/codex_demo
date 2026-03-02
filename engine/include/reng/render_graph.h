#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "reng/resources.h"
#include "reng/command_buffer.h"

namespace reng {

enum class QueueType : uint8_t {
  Graphics,
  Compute,
  Transfer,
};

enum class PassType : uint8_t {
  Blit,
  Render,
  Compute,
  ML,
};

struct PassHandle {
  uint32_t index = 0;
};

struct CompileOptions {
  bool enableAutoSync = true;
};

struct DependencyEdge {
  PassHandle from;
  PassHandle to;
  std::string reason;
  bool autoSynced = false;
};

struct PassReport {
  PassHandle handle;
  std::string name;
  PassType type = PassType::Render;
  QueueType assignedQueue = QueueType::Graphics;
  bool canRunConcurrently = false;
};

struct CompileReport {
  std::vector<PassReport> passes;
  std::vector<DependencyEdge> dependencies;
};

class BlitPassBuilder {
public:
  explicit BlitPassBuilder(CommandBuffer& cmd) : _cmd(cmd) {}
  void copyTexture(const std::string& src, const std::string& dst);
  void uploadBuffer(const std::string& name, size_t bytes);
  void uploadTexture(const std::string& name, size_t bytes);
private:
  CommandBuffer& _cmd;
};

class RenderPassBuilder {
public:
  explicit RenderPassBuilder(CommandBuffer& cmd) : _cmd(cmd) {}
  void draw(uint32_t vertexCount, uint32_t instanceCount = 1);
private:
  CommandBuffer& _cmd;
};

class ComputePassBuilder {
public:
  explicit ComputePassBuilder(CommandBuffer& cmd) : _cmd(cmd) {}
  void dispatch(uint32_t x, uint32_t y, uint32_t z);
private:
  CommandBuffer& _cmd;
};

class MLPassBuilder {
public:
  explicit MLPassBuilder(CommandBuffer& cmd) : _cmd(cmd) {}
  void dispatch(uint32_t x, uint32_t y, uint32_t z);
private:
  CommandBuffer& _cmd;
};

class RenderGraph;

class ResolvedFrame {
public:
  explicit ResolvedFrame(std::vector<CommandBuffer>&& buffers);
  void Execute();
private:
  std::vector<CommandBuffer> _buffers;
};

class RenderGraph {
public:
  void BeginFrame();

  PassHandle AddBlitPass(
      const std::string& name,
      const std::vector<ResourceAccess>& accesses,
      QueueType preferredQueue,
      const std::function<void(BlitPassBuilder&)>& record);

  PassHandle AddRenderPass(
      const std::string& name,
      const std::string& framebufferName,
      const std::vector<ResourceAccess>& accesses,
      QueueType preferredQueue,
      const std::function<void(RenderPassBuilder&)>& record);

  PassHandle AddComputePass(
      const std::string& name,
      const std::vector<ResourceAccess>& accesses,
      QueueType preferredQueue,
      const std::function<void(ComputePassBuilder&)>& record);

  PassHandle AddMLPass(
      const std::string& name,
      const std::vector<ResourceAccess>& accesses,
      QueueType preferredQueue,
      const std::function<void(MLPassBuilder&)>& record);

  CompileReport Compile(const CompileOptions& options = {});
  ResolvedFrame Resolve();

private:
  struct PassDesc {
    PassHandle handle;
    std::string name;
    PassType type;
    QueueType preferredQueue;
    std::vector<ResourceAccess> accesses;
    std::function<void(CommandBuffer&)> recordFn;
  };

  std::vector<PassDesc> _passes;
  CompileReport _lastReport;
};

} // namespace reng
