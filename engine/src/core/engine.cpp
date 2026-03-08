#include "reng/engine.h"

#include <chrono>

#include "core/resource_pool.h"
#include "platform/backend_factory.h"
#include "reng/logger.h"

namespace reng {

namespace {
uint64_t nowNs() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}
}  // namespace

Engine::Engine(const AppDesc& desc, AppCallbacks& callbacks)
    : _desc(desc), _callbacks(callbacks) {}

std::unique_ptr<Engine> Engine::create(const AppDesc& desc,
                                       AppCallbacks& callbacks,
                                       const PlatformContext& context) {
  RengLogger::logInfo("Engine::create start");
  std::unique_ptr<Engine> engine(new Engine(desc, callbacks));
  if (!engine->initBackend(context)) {
    RengLogger::logError("Failed to initialize backend");
    return nullptr;
  }
  RengLogger::logInfo("Engine::create backend initialized");

  if (!engine->_callbacks.onInit(*engine)) {
    RengLogger::logError("App initialization failed");
    return nullptr;
  }
  return engine;
}

bool Engine::initBackend(const PlatformContext& context) {
  RengLogger::logInfo("Engine::initBackend start");
  BackendBundle bundle = createBackend(_desc, context);
  _device = std::move(bundle.device);
  _swapchain = std::move(bundle.swapchain);
  _resources = std::move(bundle.resources);
  if (!_device || !_swapchain || !_resources) {
    RengLogger::logError("Engine::initBackend bundle incomplete");
    return false;
  }
  _resourcePool = std::make_unique<ResourcePoolImpl>(*_resources);
  RengLogger::logInfo("Engine::initBackend done");
  return true;
}

void Engine::tick(float deltaSeconds) {
  _lastFrameTimings.frameStartNs = nowNs();
  _callbacks.onInput();
  _callbacks.onUpdateFrame(deltaSeconds);
  _graph.beginFrame();
  _callbacks.onUpdateRender(_graph);
  _callbacks.onRender(_graph);
  _graph.compile();
  (void)_swapchain->acquireNextImage();
  auto resolved = _graph.resolve(*_device, *_resourcePool, *_swapchain);
  _lastFrameTimings.commandBuffers = resolved.execute();
  _swapchain->signalPresentReady();
  _swapchain->present();
  _lastFrameTimings.frameEndNs = nowNs();

  double frameMs = 0.0;
  if (_lastFrameTimings.frameEndNs >= _lastFrameTimings.frameStartNs) {
    frameMs = static_cast<double>(_lastFrameTimings.frameEndNs -
                                  _lastFrameTimings.frameStartNs) /
              1e6;
  }
  double gpuMs = 0.0;
  for (const auto& timing : _lastFrameTimings.commandBuffers) {
    if (timing.valid && timing.gpuEndNs >= timing.gpuStartNs) {
      gpuMs +=
          static_cast<double>(timing.gpuEndNs - timing.gpuStartNs) / 1e6;
    }
  }
  RengLogger::logVerbose("Frame time {:.3f} ms, GPU time {:.3f} ms", frameMs,
                         gpuMs);
}

}  // namespace reng
