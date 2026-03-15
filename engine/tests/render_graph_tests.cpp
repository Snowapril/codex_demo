#include <cassert>
#include <algorithm>

#include "reng/render_graph.h"

int main() {
  using namespace reng;
  RenderGraph graph;
  graph.beginFrame();

  ResourceId bufferA{1, ResourceKind::Buffer, "A"};
  ResourceId bufferB{2, ResourceKind::Buffer, "B"};

  PassHandle blitPass = graph.addBlitPass(
      "UploadA", {{bufferA, BufferAccessType::Write}}, QueueType::Transfer,
      [bufferA](BlitPassBuilder& pass) { pass.uploadBuffer(bufferA, 128); });

  PassHandle computePass = graph.addComputePass(
      "ComputeA", {{bufferA, BufferAccessType::Read}}, QueueType::Compute,
      [](ComputePassBuilder& pass) { pass.dispatch(1, 1, 1); });

  FramebufferDesc framebuffer;
  framebuffer.colorAttachments.push_back({ResourceId{3, ResourceKind::Texture,
                                                      "Color"},
                                           LoadAction::Clear,
                                           StoreAction::Store});

  PassHandle renderPass = graph.addRenderPass(
      "RenderB", framebuffer, {{bufferB, BufferAccessType::Read}},
      QueueType::Graphics, [](RenderPassBuilder& pass) { pass.draw(3); });

  auto report = graph.compile();
  assert(report.passes.size() == 3);
  assert(!report.dependencies.empty());

  auto hasDependency = [&](PassHandle from, PassHandle to) {
    return std::any_of(report.dependencies.begin(),
                       report.dependencies.end(),
                       [&](const DependencyEdge& edge) {
                         return edge.from.index == from.index &&
                                edge.to.index == to.index;
                       });
  };

  // computePass must depend on blitPass
  assert(hasDependency(blitPass, computePass));
  // renderPass must not depend on any preceding passes
  assert(!hasDependency(blitPass, renderPass));
  assert(!hasDependency(computePass, renderPass));

  return 0;
}
