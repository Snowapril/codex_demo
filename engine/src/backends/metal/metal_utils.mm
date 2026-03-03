#include "metal_utils.h"

namespace reng::metal {

MTLPixelFormat toMetalFormat(PixelFormat format) {
  switch (format) {
    case PixelFormat::Bgra8Unorm:
    default:
      return MTLPixelFormatBGRA8Unorm;
  }
}

}  // namespace reng::metal
