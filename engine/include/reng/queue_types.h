#pragma once

#include <cstdint>

namespace reng {

enum class QueueType : uint8_t {
  Graphics,
  Compute,
  Transfer,
};

}  // namespace reng
