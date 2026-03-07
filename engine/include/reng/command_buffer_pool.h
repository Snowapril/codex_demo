#pragma once

#include <memory>

#include "reng/command_buffer.h"
#include "reng/render_graph.h"

namespace reng {

class CommandBufferPool {
 public:
  virtual ~CommandBufferPool() = default;
  virtual std::unique_ptr<CommandBuffer> allocate(QueueType queueType) = 0;
};

}  // namespace reng
