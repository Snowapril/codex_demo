#include <cassert>

#include "reng/render_graph.h"

int main() {
  using namespace reng;
  RenderGraph graph;
  graph.BeginFrame();

  ResourceId bufferA{1, ResourceKind::Buffer, "A"};
  ResourceId bufferB{2, ResourceKind::Buffer, "B"};

  graph.AddBlitPass(
      "UploadA",
      {{bufferA, AccessType::Write, TextureUsage::Undefined}},
      QueueType::Transfer,
      [](BlitPassBuilder& pass) {
        pass.uploadBuffer("A", 128);
      });

  graph.AddComputePass(
      "ComputeA",
      {{bufferA, AccessType::Read, TextureUsage::Undefined}},
      QueueType::Compute,
      [](ComputePassBuilder& pass) {
        pass.dispatch(1, 1, 1);
      });

  graph.AddRenderPass(
      "RenderB",
      "framebuffer_b",
      {{bufferB, AccessType::Read, TextureUsage::Undefined}},
      QueueType::Graphics,
      [](RenderPassBuilder& pass) {
        pass.draw(3);
      });

  auto report = graph.Compile();
  assert(report.passes.size() == 3);
  assert(!report.dependencies.empty());

  return 0;
}
