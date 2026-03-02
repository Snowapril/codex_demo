#include <cassert>

#include "reng/render_graph.h"

int main() {
  using namespace reng;
  RenderGraph graph;
  graph.beginFrame();

  ResourceId bufferA{1, ResourceKind::Buffer, "A"};
  ResourceId bufferB{2, ResourceKind::Buffer, "B"};

  graph.addBlitPass("UploadA",
                    {{bufferA, AccessType::Write, TextureUsage::Undefined}},
                    QueueType::Transfer,
                    [](BlitPassBuilder& pass) { pass.uploadBuffer("A", 128); });

  graph.addComputePass(
      "ComputeA", {{bufferA, AccessType::Read, TextureUsage::Undefined}},
      QueueType::Compute,
      [](ComputePassBuilder& pass) { pass.dispatch(1, 1, 1); });

  graph.addRenderPass("RenderB", "framebuffer_b",
                      {{bufferB, AccessType::Read, TextureUsage::Undefined}},
                      QueueType::Graphics,
                      [](RenderPassBuilder& pass) { pass.draw(3); });

  auto report = graph.compile();
  assert(report.passes.size() == 3);
  assert(!report.dependencies.empty());

  return 0;
}
