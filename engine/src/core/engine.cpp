#include "reng/engine.h"

namespace reng {

Engine::Engine(const AppDesc& desc, AppCallbacks& callbacks,
               Swapchain* swapchain)
    : _desc(desc), _callbacks(callbacks), _swapchain(swapchain) {}

void Engine::tick(float deltaSeconds) {
  _callbacks.onInput();
  _callbacks.onUpdateFrame(deltaSeconds);
  _graph.beginFrame();
  _callbacks.onUpdateRender(_graph);
  _callbacks.onRender(_graph);
  _graph.compile();
  _graph.resolve().execute();
  if (_swapchain) {
    _swapchain->present();
  }
}

}  // namespace reng
