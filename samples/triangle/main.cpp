#include <iostream>

#include "reng/render_graph.h"

int main() {
  using namespace reng;

  RenderGraph graph;
  graph.beginFrame();

  ResourceId uploadBuffer{1, ResourceKind::Buffer, "upload_buffer"};
  ResourceId texture{2, ResourceKind::Texture, "color_tex"};

  graph.addBlitPass(
      "Upload",
      {
          {uploadBuffer, AccessType::Write, TextureUsage::Undefined},
          {texture, AccessType::Write, TextureUsage::TransferDst},
      },
      QueueType::Transfer, [](BlitPassBuilder& pass) {
        pass.uploadBuffer("upload_buffer", 1024);
        pass.uploadTexture("color_tex", 4096);
      });

  graph.addRenderPass("Render", "framebuffer_main",
                      {
                          {texture, AccessType::Read, TextureUsage::Sampled},
                      },
                      QueueType::Graphics,
                      [](RenderPassBuilder& pass) { pass.draw(3, 1); });

  graph.addComputePass(
      "Compute",
      {
          {texture, AccessType::ReadWrite, TextureUsage::Storage},
      },
      QueueType::Compute,
      [](ComputePassBuilder& pass) { pass.dispatch(8, 8, 1); });

  auto report = graph.compile();

  std::cout << "Compile report: " << report.passes.size() << " passes, "
            << report.dependencies.size() << " deps\n";

  auto frame = graph.resolve();
  frame.execute();

  return 0;
}
