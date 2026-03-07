#include "reng/app.h"
#include "reng/engine.h"
#include "reng/logger.h"
#include "reng/resources.h"

class BlankApp : public reng::AppCallbacks {
 public:
  BlankApp() : _seed(0x12345678u) {}

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
    reng::FramebufferAttachment colorAttachment;
    colorAttachment.resource = swapchainColor;
    colorAttachment.loadAction = reng::LoadAction::Clear;
    colorAttachment.storeAction = reng::StoreAction::Store;
    colorAttachment.clearColor = randomClearColor();
    framebuffer.colorAttachments.push_back(colorAttachment);
    graph.addRenderPass("Blank", framebuffer,
                        {
                            {swapchainColor,
                             reng::TextureAccessType::RenderTarget},
                        },
                        reng::QueueType::Graphics,
                        [](reng::RenderPassBuilder& pass) { pass.draw(0); });
  }

 private:
  glm::vec4 randomClearColor() {
    return {nextFloat(), nextFloat(), nextFloat(), 1.0f};
  }

  float nextFloat() {
    _seed = 1664525u * _seed + 1013904223u;
    uint32_t value = (_seed >> 8) & 0x00FFFFFFu;
    return static_cast<float>(value) / 16777215.0f;
  }

  const reng::ResourceId swapchainColor{
      1, reng::ResourceKind::Texture, "swapchain_color"};
  uint32_t _seed = 0;
};

int main(int argc, char** argv) {
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
  return reng::runApp(argc, argv, desc, app);
}
