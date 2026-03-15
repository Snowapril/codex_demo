#pragma once

#include <cstdint>

#include "reng/device.h"
#include "reng/render_graph.h"

namespace reng {

enum class Backend : uint8_t {
  Metal,
  Vulkan,
};

enum class PixelFormat : uint8_t {
  Bgra8Unorm,
};

enum class PresentMode : uint8_t {
  Vsync,
  Immediate,
};

struct SwapchainDesc {
  uint32_t width = 800;
  uint32_t height = 600;
  PixelFormat colorFormat = PixelFormat::Bgra8Unorm;
  PresentMode presentMode = PresentMode::Vsync;
};

struct AppDesc {
  Backend backend = Backend::Vulkan;
  const char* title = "Reng engine";
  SwapchainDesc swapchain;
  DeviceDesc device;
  float maxRunSeconds = 0.0f;
};

class Engine;

class AppCallbacks {
 public:
  virtual ~AppCallbacks() = default;
  virtual bool onInit(Engine& engine) { return true; }
  virtual void onInput() {}
  virtual void onUpdateFrame(float deltaSeconds) {}
  virtual void onUpdateRender(RenderGraph& graph) { (void)graph; }
  virtual void onRender(RenderGraph& graph) { (void)graph; }
  virtual bool shouldExit() const { return false; }
};

int runApp(const AppDesc& desc, AppCallbacks& callbacks);
int runApp(int argc, char** argv, const AppDesc& desc, AppCallbacks& callbacks);

}  // namespace reng
