#include "reng/engine.h"

#include "core/resource_pool.h"
#include "platform/backend_factory.h"
#include "reng/logger.h"

namespace reng {

Engine::Engine(const AppDesc& desc, AppCallbacks& callbacks)
    : _desc(desc), _callbacks(callbacks) {}

std::unique_ptr<Engine> Engine::create(const AppDesc& desc,
                                       AppCallbacks& callbacks,
                                       const PlatformContext& context) {
  std::unique_ptr<Engine> engine(new Engine(desc, callbacks));
  if (!engine->initBackend(context)) {
    RengLogger::logError("Failed to initialize backend");
    return nullptr;
  }

  if (!engine->_callbacks.onInit(*engine)) {
    RengLogger::logError("App initialization failed");
    return nullptr;
  }
  return engine;
}

bool Engine::initBackend(const PlatformContext& context) {
  BackendBundle bundle = createBackend(_desc, context);
  _device = std::move(bundle.device);
  _swapchain = std::move(bundle.swapchain);
  _resources = std::move(bundle.resources);
  if (!_device || !_swapchain || !_resources) {
    return false;
  }
  _resourcePool = std::make_unique<ResourcePoolImpl>(*_resources);
  return true;
}

void Engine::tick(float deltaSeconds) {
  _callbacks.onInput();
  _callbacks.onUpdateFrame(deltaSeconds);
  _graph.beginFrame();
  _callbacks.onUpdateRender(_graph);
  _callbacks.onRender(_graph);
  _graph.compile();
  (void)_swapchain->acquireNextImage();
  _graph.resolve(*_device).execute();
  _swapchain->signalPresentReady();
  _swapchain->present();
}

}  // namespace reng
