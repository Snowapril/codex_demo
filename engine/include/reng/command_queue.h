#pragma once

#include <memory>

#include "reng/queue_timeline.h"

namespace reng {

class BackendDevice;

class CommandQueue {
 public:
  virtual ~CommandQueue() = default;

  bool init(BackendDevice& device, QueueType type) {
    _device = &device;
    _type = type;
    _timeline = createTimeline(device);
    if (!_timeline || !_timeline->init(type)) {
      return false;
    }
    return initInner();
  }

  void shutdown() {
    shutdownInner();
    if (_timeline) {
      _timeline->shutdown();
      _timeline.reset();
    }
    _device = nullptr;
  }

  QueueTimeline* timeline() const { return _timeline.get(); }

 protected:
  BackendDevice& device() const { return *_device; }
  QueueType type() const { return _type; }

  virtual std::unique_ptr<QueueTimeline> createTimeline(
      BackendDevice& device) = 0;
  virtual bool initInner() = 0;
  virtual void shutdownInner() = 0;

 private:
  BackendDevice* _device = nullptr;
  QueueType _type = QueueType::Graphics;
  std::unique_ptr<QueueTimeline> _timeline;
};

}  // namespace reng
