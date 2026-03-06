#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace reng {

enum class ResourceKind : uint8_t {
  Buffer,
  Texture,
};

enum class MemoryType : uint8_t {
  DeviceLocal,
  HostVisible,
};

enum class BufferAccessType : uint8_t {
  Read,
  Write,
  ReadWrite,
};

enum class TextureAccessType : uint8_t {
  Undefined,
  RenderTarget,
  Sampled,
  Storage,
  TransferSrc,
  TransferDst,
};

using AccessType = std::variant<BufferAccessType, TextureAccessType>;

enum class LoadAction : uint8_t {
  Load,
  Clear,
  DontCare,
};

enum class StoreAction : uint8_t {
  Store,
  DontCare,
};

struct ResourceId {
  uint32_t value = 0;
  ResourceKind kind = ResourceKind::Buffer;
  std::string name;

  bool operator==(const ResourceId& other) const {
    return value == other.value;
  }
  bool operator!=(const ResourceId& other) const { return !(*this == other); }
};

struct ResourceAccess {
  ResourceId resource;
  AccessType access;
};

struct FramebufferAttachment {
  ResourceId resource;
  LoadAction loadAction = LoadAction::Load;
  StoreAction storeAction = StoreAction::Store;
};

struct FramebufferDesc {
  std::vector<FramebufferAttachment> colorAttachments;
  std::optional<FramebufferAttachment> depthStencilAttachment;
};

class ResourcePool {
 public:
  virtual ~ResourcePool() = default;
  virtual void* lookupBuffer(const ResourceId& id) = 0;
  virtual void* lookupTexture(const ResourceId& id) = 0;
};

}  // namespace reng
