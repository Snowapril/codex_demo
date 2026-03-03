#include "reng/app.h"
#include "reng/logger.h"

class BlankApp : public reng::AppCallbacks {
 public:
  BlankApp() = default;

  void onUpdateRender(reng::RenderGraph& graph) override { (void)graph; }

  void onRender(reng::RenderGraph& graph) override {
    // Empty render pass: the backend will clear/present.
    graph.addRenderPass(
        "Blank", "swapchain",
        {
            {reng::ResourceId{1, reng::ResourceKind::Texture, "color"},
             reng::AccessType::Write, reng::TextureUsage::RenderTarget},
        },
        reng::QueueType::Graphics,
        [](reng::RenderPassBuilder& pass) { pass.draw(0); });
  }
};

int main() {
  reng::RengLogger::init("logs/blank.log");
  reng::RengLogger::logInfo("Launching blank sample");
  reng::AppDesc desc;
#if defined(__APPLE__)
  desc.backend = reng::Backend::Metal;
#else
  desc.backend = reng::Backend::Vulkan;
#endif
  desc.title = "Blank Sample";
  desc.swapchain.width = 800;
  desc.swapchain.height = 600;
  desc.swapchain.colorFormat = reng::PixelFormat::Bgra8Unorm;

  BlankApp app;
  return reng::runApp(desc, app);
}
