#pragma once

#include <memory>

#include "reng/app.h"
#include "reng/backend.h"
#include "reng/platform.h"

namespace reng {

struct BackendBundle {
  std::unique_ptr<BackendDevice> device;
  std::unique_ptr<BackendSwapchain> swapchain;
  std::unique_ptr<BackendResources> resources;
};

BackendBundle createBackend(const AppDesc& desc, const PlatformContext& context);

}  // namespace reng
