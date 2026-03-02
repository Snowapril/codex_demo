#pragma once

#include <memory>
#include <string>

namespace reng {

struct DeviceDesc {
  bool enableValidation = false;
};

class Device {
public:
  explicit Device(const DeviceDesc& desc);
  const DeviceDesc& desc() const { return _desc; }

private:
  DeviceDesc _desc;
};

} // namespace reng
