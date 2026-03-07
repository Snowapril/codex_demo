#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <glm/vec4.hpp>

namespace reng {

enum class PixelFormat : uint8_t;

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

enum class TextureType : uint8_t {
  Texture2D,
};

enum class TextureUsage : uint32_t {
  None = 0,
  RenderTarget = 1u << 0,
  Sampled = 1u << 1,
  Storage = 1u << 2,
  TransferSrc = 1u << 3,
  TransferDst = 1u << 4,
  Present = 1u << 5,
};

inline TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) {
  return static_cast<TextureUsage>(static_cast<uint32_t>(lhs) |
                                   static_cast<uint32_t>(rhs));
}

inline TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) {
  return static_cast<TextureUsage>(static_cast<uint32_t>(lhs) &
                                   static_cast<uint32_t>(rhs));
}

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
  glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepth = 1.0f;
  uint8_t clearStencil = 0;
};

struct FramebufferDesc {
  std::vector<FramebufferAttachment> colorAttachments;
  std::optional<FramebufferAttachment> depthStencilAttachment;
};

struct BufferCreateDesc {
  size_t bytes = 0;
  MemoryType memoryType = MemoryType::DeviceLocal;
};

struct TextureCreateDesc {
  uint32_t width = 1;
  uint32_t height = 1;
  PixelFormat format{};
  uint32_t slices = 1;
  uint32_t mips = 1;
  TextureType type = TextureType::Texture2D;
  TextureUsage usage = TextureUsage::Sampled;
};

class ResourcePool {
 public:
  virtual ~ResourcePool() = default;
  virtual bool addBuffer(const ResourceId& id,
                         const BufferCreateDesc& desc) = 0;
  virtual bool addTexture(const ResourceId& id,
                          const TextureCreateDesc& desc) = 0;
  virtual bool addSwapchainTexture(const ResourceId& id,
                                   const TextureCreateDesc& desc) = 0;
  virtual void* lookupBuffer(const ResourceId& id) = 0;
  virtual void* lookupTexture(const ResourceId& id) = 0;
};

}  // namespace reng
