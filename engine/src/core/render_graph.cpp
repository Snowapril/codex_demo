#include "reng/render_graph.h"

#include <unordered_map>

#include "reng/command_buffer.h"

namespace reng {

namespace {
QueueType defaultQueueForPass(PassType type) {
  switch (type) {
    case PassType::Blit:
      return QueueType::Transfer;
    case PassType::Compute:
      return QueueType::Compute;
    case PassType::ML:
      return QueueType::Compute;
    case PassType::Render:
    default:
      return QueueType::Graphics;
  }
}

bool isWriteAccess(AccessType access) {
  return access == AccessType::Write || access == AccessType::ReadWrite;
}
}  // namespace

void BlitPassBuilder::copyTexture(const std::string& src,
                                  const std::string& dst) {
  _cmd.copyTexture(src, dst);
}

void BlitPassBuilder::uploadBuffer(const std::string& name, size_t bytes) {
  _cmd.uploadBuffer(name, bytes);
}

void BlitPassBuilder::uploadTexture(const std::string& name, size_t bytes) {
  _cmd.uploadTexture(name, bytes);
}

void RenderPassBuilder::draw(uint32_t vertexCount, uint32_t instanceCount) {
  _cmd.draw(vertexCount, instanceCount);
}

void ComputePassBuilder::dispatch(uint32_t x, uint32_t y, uint32_t z) {
  _cmd.dispatch(x, y, z);
}

void MLPassBuilder::dispatch(uint32_t x, uint32_t y, uint32_t z) {
  _cmd.dispatchML(x, y, z);
}

ResolvedFrame::ResolvedFrame(std::vector<CommandBuffer>&& buffers)
    : _buffers(std::move(buffers)) {}

void ResolvedFrame::execute() {
  // Placeholder: in a full implementation this would submit to platform queues.
  // We intentionally leave this as a no-op for now.
}

void RenderGraph::beginFrame() {
  _passes.clear();
  _lastReport = {};
}

PassHandle RenderGraph::addBlitPass(
    const std::string& name, const std::vector<ResourceAccess>& accesses,
    QueueType preferredQueue,
    const std::function<void(BlitPassBuilder&)>& record) {
  PassHandle handle{static_cast<uint32_t>(_passes.size())};
  PassDesc desc;
  desc.handle = handle;
  desc.name = name;
  desc.type = PassType::Blit;
  desc.preferredQueue = preferredQueue;
  desc.accesses = accesses;
  desc.recordFn = [record](CommandBuffer& cmd) {
    cmd.beginBlitPass();
    BlitPassBuilder builder(cmd);
    record(builder);
    cmd.endBlitPass();
  };
  _passes.push_back(desc);
  return handle;
}

PassHandle RenderGraph::addRenderPass(
    const std::string& name, const std::string& framebufferName,
    const std::vector<ResourceAccess>& accesses, QueueType preferredQueue,
    const std::function<void(RenderPassBuilder&)>& record) {
  PassHandle handle{static_cast<uint32_t>(_passes.size())};
  PassDesc desc;
  desc.handle = handle;
  desc.name = name;
  desc.type = PassType::Render;
  desc.preferredQueue = preferredQueue;
  desc.accesses = accesses;
  desc.recordFn = [record, framebufferName](CommandBuffer& cmd) {
    cmd.beginRenderPass(framebufferName);
    RenderPassBuilder builder(cmd);
    record(builder);
    cmd.endRenderPass();
  };
  _passes.push_back(desc);
  return handle;
}

PassHandle RenderGraph::addComputePass(
    const std::string& name, const std::vector<ResourceAccess>& accesses,
    QueueType preferredQueue,
    const std::function<void(ComputePassBuilder&)>& record) {
  PassHandle handle{static_cast<uint32_t>(_passes.size())};
  PassDesc desc;
  desc.handle = handle;
  desc.name = name;
  desc.type = PassType::Compute;
  desc.preferredQueue = preferredQueue;
  desc.accesses = accesses;
  desc.recordFn = [record](CommandBuffer& cmd) {
    cmd.beginComputePass();
    ComputePassBuilder builder(cmd);
    record(builder);
    cmd.endComputePass();
  };
  _passes.push_back(desc);
  return handle;
}

PassHandle RenderGraph::addMLPass(
    const std::string& name, const std::vector<ResourceAccess>& accesses,
    QueueType preferredQueue,
    const std::function<void(MLPassBuilder&)>& record) {
  PassHandle handle{static_cast<uint32_t>(_passes.size())};
  PassDesc desc;
  desc.handle = handle;
  desc.name = name;
  desc.type = PassType::ML;
  desc.preferredQueue = preferredQueue;
  desc.accesses = accesses;
  desc.recordFn = [record](CommandBuffer& cmd) {
    cmd.beginMLPass();
    MLPassBuilder builder(cmd);
    record(builder);
    cmd.endMLPass();
  };
  _passes.push_back(desc);
  return handle;
}

CompileReport RenderGraph::compile(const CompileOptions& options) {
  CompileReport report;
  report.passes.reserve(_passes.size());

  std::unordered_map<uint32_t, PassHandle> lastWriter;
  std::unordered_map<uint32_t, std::vector<PassHandle>> lastReaders;

  auto addDependency = [&](PassHandle from, PassHandle to,
                           const std::string& reason, bool autoSynced) {
    report.dependencies.push_back({from, to, reason, autoSynced});
  };

  for (const auto& pass : _passes) {
    PassReport pr;
    pr.handle = pass.handle;
    pr.name = pass.name;
    pr.type = pass.type;
    pr.assignedQueue = pass.preferredQueue;
    if (pr.assignedQueue == QueueType::Graphics &&
        pass.type != PassType::Render) {
      pr.assignedQueue = defaultQueueForPass(pass.type);
    }

    bool hasDependency = false;
    for (const auto& access : pass.accesses) {
      const uint32_t key = access.resource.value;
      const bool write = isWriteAccess(access.access);

      if (lastWriter.count(key)) {
        PassHandle from = lastWriter[key];
        if (from.index != pass.handle.index) {
          addDependency(from, pass.handle, "Write-after-read/write hazard",
                        options.enableAutoSync);
          hasDependency = true;
        }
      }

      if (write) {
        for (const auto& reader : lastReaders[key]) {
          if (reader.index != pass.handle.index) {
            addDependency(reader, pass.handle, "Read-after-write hazard",
                          options.enableAutoSync);
            hasDependency = true;
          }
        }
        lastReaders[key].clear();
        lastWriter[key] = pass.handle;
      } else {
        lastReaders[key].push_back(pass.handle);
      }
    }

    pr.canRunConcurrently = !hasDependency;
    report.passes.push_back(pr);
  }

  _lastReport = report;
  return report;
}

ResolvedFrame RenderGraph::resolve() {
  std::vector<CommandBuffer> buffers;
  buffers.reserve(_passes.size());
  for (const auto& pass : _passes) {
    CommandBuffer cmd;
    if (pass.recordFn) {
      pass.recordFn(cmd);
    }
    buffers.push_back(std::move(cmd));
  }
  return ResolvedFrame(std::move(buffers));
}

}  // namespace reng
