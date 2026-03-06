#include <iostream>

#include "reng/render_graph.h"

int main() {
  using namespace reng;

  RenderGraph graph;
  graph.beginFrame();

  ResourceId uploadBuffer{1, ResourceKind::Buffer, "upload_buffer"};
  ResourceId texture{2, ResourceKind::Texture, "color_tex"};
  ResourceId swapchainColor{3, ResourceKind::Texture, "swapchain_color"};

  graph.addBlitPass(
      "Upload",
      {
          {uploadBuffer, BufferAccessType::Write},
          {texture, TextureAccessType::TransferDst},
      },
      QueueType::Transfer, [uploadBuffer, texture](BlitPassBuilder& pass) {
        pass.uploadBuffer(uploadBuffer, 1024);
        pass.uploadTexture(texture, 4096);
      });

  FramebufferDesc framebuffer;
  framebuffer.colorAttachments.push_back(
      {swapchainColor, LoadAction::Clear, StoreAction::Store});
  graph.addRenderPass("Render", framebuffer,
                      {
                          {texture, TextureAccessType::Sampled},
                      },
                      QueueType::Graphics,
                      [](RenderPassBuilder& pass) { pass.draw(3, 1); });

  graph.addComputePass(
      "Compute",
      {
          {texture, TextureAccessType::Storage},
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
