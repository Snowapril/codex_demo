#pragma once

#include <cstdint>
#include <memory>

#include "reng/app.h"
#include "reng/backend.h"
#include "reng/platform.h"
#include "reng/render_graph.h"

namespace reng {

class Engine {
 public:
  static std::unique_ptr<Engine> create(const AppDesc& desc,
                                        AppCallbacks& callbacks,
                                        const PlatformContext& context);

  void tick(float deltaSeconds);

 private:
  Engine(const AppDesc& desc, AppCallbacks& callbacks);
  bool initBackend(const PlatformContext& context);

  AppDesc _desc;
  AppCallbacks& _callbacks;
  RenderGraph _graph;
  std::unique_ptr<BackendDevice> _device;
  std::unique_ptr<BackendSwapchain> _swapchain;
};

}  // namespace reng
