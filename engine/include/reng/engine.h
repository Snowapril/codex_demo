#pragma once

#include <cstdint>

#include "reng/app.h"
#include "reng/render_graph.h"
#include "reng/swapchain.h"

namespace reng {

class Engine {
 public:
  Engine(const AppDesc& desc, AppCallbacks& callbacks, Swapchain* swapchain);

  void tick(float deltaSeconds);

 private:
  AppDesc _desc;
  AppCallbacks& _callbacks;
  RenderGraph _graph;
  Swapchain* _swapchain = nullptr;
};

}  // namespace reng
