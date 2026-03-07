#include "reng/app.h"
#include "reng/engine.h"
#include "reng/logger.h"

class TriangleApp : public reng::AppCallbacks {
 public:
  TriangleApp() = default;

  bool onInit(reng::Engine& engine) override {
    reng::ResourcePool* pool = engine.resourcePool();
    pool->addSwapchainTexture(swapchainColor, reng::TextureCreateDesc{
        .width = engine.swapchain()->width(),
        .height = engine.swapchain()->height(),
        .format = engine.swapchain()->colorFormat(),
        .slices = 1,
        .mips = 1,
        .type = reng::TextureType::Texture2D,
        .usage = reng::TextureUsage::RenderTarget | reng::TextureUsage::Present,
    });
    return true;
  }

  void onUpdateRender(reng::RenderGraph& graph) override { (void)graph; }

  void onRender(reng::RenderGraph& graph) override {
    // Empty render pass: the backend will clear/present.
    reng::FramebufferDesc framebuffer;
    framebuffer.colorAttachments.push_back({swapchainColor,
                                            reng::LoadAction::Clear,
                                            reng::StoreAction::Store});
    graph.addRenderPass("Triangle", framebuffer,
                        {
                            {swapchainColor,
                             reng::TextureAccessType::RenderTarget},
                        },
                        reng::QueueType::Graphics,
                        [](reng::RenderPassBuilder& pass) { pass.draw(0); });
  }

 private:
  const reng::ResourceId swapchainColor{
      1, reng::ResourceKind::Texture, "swapchain_color"};
};

int main(int argc, char** argv) {
  reng::RengLogger::init("logs/triangle.log");
  reng::RengLogger::logInfo("Launching triangle sample");
  reng::AppDesc desc;
#if defined(__APPLE__)
  desc.backend = reng::Backend::Metal;
#else
  desc.backend = reng::Backend::Vulkan;
#endif
  desc.title = "Triangle Sample";
  desc.swapchain.width = 800;
  desc.swapchain.height = 600;
  desc.swapchain.colorFormat = reng::PixelFormat::Bgra8Unorm;

  TriangleApp app;
  return reng::runApp(argc, argv, desc, app);
}
