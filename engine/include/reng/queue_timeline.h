#pragma once

#include <cstdint>

#include "reng/queue_types.h"

namespace reng {

class BackendDevice;

struct TimelineHandle {
  class QueueTimeline* timeline = nullptr;
  uint64_t value = 0;
};

class QueueTimeline {
 public:
  explicit QueueTimeline(BackendDevice& device) : _device(device) {}
  virtual ~QueueTimeline() = default;

  bool init(QueueType type) {
    _type = type;
    _lastAllocatedValue = 0;
    return initInner();
  }

  void shutdown() { shutdownInner(); }

  uint64_t lastAllocated() const { return _lastAllocatedValue; }
  TimelineHandle allocateNext() {
    return TimelineHandle{this, nextValue()};
  }
  virtual uint64_t completedValue() const = 0;

 protected:
  virtual bool initInner() = 0;
  virtual void shutdownInner() = 0;

private:
  uint64_t nextValue() { return ++_lastAllocatedValue; }

 protected:
  BackendDevice& _device;
  uint64_t _lastAllocatedValue = 0;
  QueueType _type;
};

}  // namespace reng
