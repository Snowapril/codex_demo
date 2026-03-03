#pragma once

#include "reng/app.h"

namespace reng {

class BackendDevice {
 public:
  virtual ~BackendDevice() = default;
  virtual void shutdown() = 0;
};

class BackendSwapchain {
 public:
  virtual ~BackendSwapchain() = default;
  virtual bool recreate(const SwapchainDesc& desc) = 0;
  virtual void present() = 0;
};

class BackendResources {
 public:
  virtual ~BackendResources() = default;
};

class BackendCommand {
 public:
  virtual ~BackendCommand() = default;
};

}  // namespace reng
