#pragma once

#include <memory>

#include "reng/command_buffer.h"
#include "reng/render_graph.h"

namespace reng {

class CommandQueue;

class CommandBufferPool {
 public:
  explicit CommandBufferPool(CommandQueue& queue) : _queue(queue) {}
  virtual ~CommandBufferPool() = default;

  virtual std::unique_ptr<CommandBuffer> allocate() = 0;

 protected:
  CommandQueue& queue() const { return _queue; }

 private:
  CommandQueue& _queue;
};

}  // namespace reng
