#pragma once

#import <Metal/Metal.h>

#include "reng/app.h"

namespace reng::metal {

MTLPixelFormat toMetalFormat(PixelFormat format);
PixelFormat fromMetalFormat(MTLPixelFormat format);

}  // namespace reng::metal
