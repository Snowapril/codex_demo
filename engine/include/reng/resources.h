#pragma once

#include <cstdint>
#include <string>

namespace reng {

enum class ResourceKind : uint8_t {
  Buffer,
  Texture,
};

enum class AccessType : uint8_t {
  Read,
  Write,
  ReadWrite,
};

enum class TextureUsage : uint8_t {
  Undefined,
  RenderTarget,
  Sampled,
  Storage,
  TransferSrc,
  TransferDst,
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
  AccessType access = AccessType::Read;
  TextureUsage usage = TextureUsage::Undefined;
};

}  // namespace reng
