#pragma once

#include <cstdint>
#include <memory>

#include "reng/app.h"
#include "reng/backend.h"
#include "reng/platform.h"
#include "reng/render_graph.h"
#include "reng/resources.h"

namespace reng {

class Engine {
 public:
  static std::unique_ptr<Engine> create(const AppDesc& desc,
                                        AppCallbacks& callbacks,
                                        const PlatformContext& context);

  void tick(float deltaSeconds);
  ResourcePool* resourcePool() { return _resourcePool.get(); }
  BackendSwapchain* swapchain() { return _swapchain.get(); }

 private:
  Engine(const AppDesc& desc, AppCallbacks& callbacks);
  bool initBackend(const PlatformContext& context);

  AppDesc _desc;
  AppCallbacks& _callbacks;
  RenderGraph _graph;
  std::unique_ptr<BackendDevice> _device;
  std::unique_ptr<BackendSwapchain> _swapchain;
  std::unique_ptr<BackendResources> _resources;
  std::unique_ptr<ResourcePool> _resourcePool;
};

}  // namespace reng
