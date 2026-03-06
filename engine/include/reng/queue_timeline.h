#pragma once

#include <cstdint>

namespace reng {

class QueueTimeline {
 public:
  virtual ~QueueTimeline() = default;

  uint64_t currentValue() const { return _value; }
  uint64_t nextValue() { return ++_value; }

 protected:
  uint64_t _value = 0;
};

}  // namespace reng
