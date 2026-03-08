#include "reng/render_graph.h"

#include <array>
#include <unordered_map>

#include "reng/command_buffer.h"
#include "reng/backend.h"

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

bool isWriteAccess(const ResourceAccess& access) {
  if (std::holds_alternative<BufferAccessType>(access.access)) {
    auto mode = std::get<BufferAccessType>(access.access);
    return mode == BufferAccessType::Write ||
           mode == BufferAccessType::ReadWrite;
  }
  auto mode = std::get<TextureAccessType>(access.access);
  return mode == TextureAccessType::RenderTarget ||
         mode == TextureAccessType::Storage ||
         mode == TextureAccessType::TransferDst;
}

}  // namespace

void BlitPassBuilder::copyTexture(const ResourceId& src,
                                  const ResourceId& dst) {
  _cmd.copyTexture(src, dst);
}

void BlitPassBuilder::uploadBuffer(const ResourceId& buffer, size_t bytes) {
  _cmd.uploadBuffer(buffer, bytes);
}

void BlitPassBuilder::uploadTexture(const ResourceId& texture, size_t bytes) {
  _cmd.uploadTexture(texture, bytes);
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

ResolvedFrame::ResolvedFrame(std::vector<std::unique_ptr<CommandBuffer>>&& buffers,
                             std::vector<QueueType>&& queueTypes)
    : _buffers(std::move(buffers)),
      _queueTypes(std::move(queueTypes)) {}

std::vector<CommandBufferTiming> ResolvedFrame::execute() {
  std::vector<CommandBufferTiming> timings;
  timings.reserve(_buffers.size());
  for (size_t i = 0; i < _buffers.size(); ++i) {
    auto& buffer = _buffers[i];
    if (!buffer) {
      continue;
    }
    timings.push_back(buffer->submit());
  }
  return timings;
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
    const std::string& name, const FramebufferDesc& framebuffer,
    const std::vector<ResourceAccess>& accesses, QueueType preferredQueue,
    const std::function<void(RenderPassBuilder&)>& record) {
  PassHandle handle{static_cast<uint32_t>(_passes.size())};
  PassDesc desc;
  desc.handle = handle;
  desc.name = name;
  desc.type = PassType::Render;
  desc.preferredQueue = preferredQueue;
  desc.accesses = accesses;
  desc.framebuffer = framebuffer;
  desc.recordFn = [record, framebuffer](CommandBuffer& cmd) {
    cmd.beginRenderPass(framebuffer);
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
  _lastOptions = options;
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
      const bool write = isWriteAccess(access);

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

ResolvedFrame RenderGraph::resolve(BackendDevice& device,
                                   ResourcePool& resources,
                                   BackendSwapchain& swapchain) {
  if (_lastReport.passes.empty()) {
    compile();
  }

  const size_t passCount = _passes.size();
  std::vector<bool> graphicsPass(passCount, false);
  for (const auto& pass : _passes) {
    if (pass.preferredQueue == QueueType::Graphics ||
        pass.type == PassType::Render) {
      graphicsPass[pass.handle.index] = true;
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto& pass : _passes) {
      if (graphicsPass[pass.handle.index]) {
        continue;
      }
      if (pass.preferredQueue == QueueType::Compute ||
          pass.preferredQueue == QueueType::Transfer) {
        for (const auto& edge : _lastReport.dependencies) {
          const bool linked =
              (edge.from.index == pass.handle.index &&
               graphicsPass[edge.to.index]) ||
              (edge.to.index == pass.handle.index &&
               graphicsPass[edge.from.index]);
          if (linked) {
            graphicsPass[pass.handle.index] = true;
            changed = true;
            break;
          }
        }
      }
    }
  }

  auto selectQueue = [&](QueueType type) -> CommandQueue* {
    if (type == QueueType::Graphics) {
      return device.graphicsQueue();
    }
    if (type == QueueType::Compute) {
      return device.computeQueue();
    }
    return device.copyQueue(0);
  };

  std::unordered_map<QueueType, std::unique_ptr<CommandBuffer>> queueBuffers;
  queueBuffers.reserve(3);

  for (const auto& pass : _passes) {
    QueueType assignedQueue = QueueType::Graphics;
    if (!graphicsPass[pass.handle.index] &&
        (pass.preferredQueue == QueueType::Compute ||
         pass.preferredQueue == QueueType::Transfer)) {
      assignedQueue = pass.preferredQueue;
    }

    auto& cmd = queueBuffers[assignedQueue];
    if (!cmd) {
      CommandQueue* queue = selectQueue(assignedQueue);
      if (!queue || !queue->commandBufferPool()) {
        continue;
      }
      cmd = queue->commandBufferPool()->allocate();
      if (cmd) {
        cmd->setContext(&resources, &swapchain);
      }
    }

    if (pass.recordFn) {
      if (!cmd->isRecording()) {
        cmd->beginCommandBuffer(_lastOptions.enableTimestamps);
      }
      pass.recordFn(*cmd);
    }
  }

  std::vector<std::unique_ptr<CommandBuffer>> buffers;
  std::vector<QueueType> queueTypes;
  buffers.reserve(queueBuffers.size());
  queueTypes.reserve(queueBuffers.size());

  const std::array<QueueType, 3> queueOrder = {
      QueueType::Graphics, QueueType::Compute, QueueType::Transfer};
  for (QueueType queueType : queueOrder) {
    auto it = queueBuffers.find(queueType);
    if (it == queueBuffers.end() || !it->second) {
      continue;
    }
    if (it->second->isRecording()) {
      it->second->endCommandBuffer();
    }
    buffers.push_back(std::move(it->second));
    queueTypes.push_back(queueType);
  }
  return ResolvedFrame(std::move(buffers), std::move(queueTypes));
}

}  // namespace reng
