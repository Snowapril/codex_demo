#pragma once

#include <cstdint>

#include "reng/render_graph.h"

namespace reng {

enum class Backend : uint8_t {
  Metal,
  Vulkan,
};

enum class PixelFormat : uint8_t {
  Bgra8Unorm,
};

struct SwapchainDesc {
  uint32_t width = 800;
  uint32_t height = 600;
  PixelFormat colorFormat = PixelFormat::Bgra8Unorm;
};

struct AppDesc {
  Backend backend = Backend::Vulkan;
  const char* title = "Blank Sample";
  SwapchainDesc swapchain;
};

class AppCallbacks {
 public:
  virtual ~AppCallbacks() = default;
  virtual void onInput() {}
  virtual void onUpdateFrame(float deltaSeconds) {}
  virtual void onUpdateRender(RenderGraph& graph) { (void)graph; }
  virtual void onRender(RenderGraph& graph) { (void)graph; }
};

int runApp(const AppDesc& desc, AppCallbacks& callbacks);

}  // namespace reng
