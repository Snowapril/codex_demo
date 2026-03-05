#include "reng/engine.h"

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
  return engine;
}

bool Engine::initBackend(const PlatformContext& context) {
  BackendBundle bundle = createBackend(_desc, context);
  _device = std::move(bundle.device);
  _swapchain = std::move(bundle.swapchain);
  return _device && _swapchain;
}

void Engine::tick(float deltaSeconds) {
  _callbacks.onInput();
  _callbacks.onUpdateFrame(deltaSeconds);
  _graph.beginFrame();
  _callbacks.onUpdateRender(_graph);
  _callbacks.onRender(_graph);
  _graph.compile();
  _graph.resolve().execute();
  _swapchain->present();
}

}  // namespace reng
