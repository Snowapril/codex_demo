#include <cstdlib>
#include <string>

#include "reng/app.h"
#include "reng/logger.h"

namespace {
class SwapchainSmokeApp : public reng::AppCallbacks {
 public:
  explicit SwapchainSmokeApp(int frames) : _targetFrames(frames) {}

  void onUpdateFrame(float deltaSeconds) override {
    (void)deltaSeconds;
    ++_frameCount;
  }

  bool shouldExit() const override { return _frameCount >= _targetFrames; }

 private:
  int _frameCount = 0;
  int _targetFrames = 3;
};
}  // namespace

int main() {
  using namespace reng;
  const char* headless = std::getenv("RENG_HEADLESS");
  if (headless && std::string(headless) == "1") {
    RengLogger::init("logs/tests_swapchain_smoke.log");
    RengLogger::logWarning("Headless mode enabled; skipping swapchain test");
    RengLogger::shutdown();
    return 0;
  }
  RengLogger::init("logs/tests_swapchain_smoke.log");
  RengLogger::logInfo("Starting swapchain smoke test");

  AppDesc desc;
#if defined(__APPLE__)
  desc.backend = Backend::Metal;
#else
  desc.backend = Backend::Vulkan;
#endif
  desc.title = "Swapchain Smoke Test";
  desc.swapchain.width = 320;
  desc.swapchain.height = 240;
  desc.swapchain.colorFormat = PixelFormat::Bgra8Unorm;
  desc.swapchain.presentMode = PresentMode::Vsync;
  desc.maxRunSeconds = 2.0f;

  SwapchainSmokeApp app(3);
  int result = runApp(desc, app);

  if (result == 0) {
    RengLogger::logInfo("Swapchain smoke test passed");
  } else {
    RengLogger::logError("Swapchain smoke test failed with code {}", result);
  }
  RengLogger::shutdown();
  return result;
}
