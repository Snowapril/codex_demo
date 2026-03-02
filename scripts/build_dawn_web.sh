#!/bin/sh
set -e

DAWN_DIR=${DAWN_DIR:-third_party/dawn}
BUILD_DIR=${BUILD_DIR:-third_party/dawn_out/web}
EMSDK=${EMSDK:-$HOME/emsdk}

if [ ! -d "$DAWN_DIR" ]; then
  echo "Dawn not found at $DAWN_DIR. Run scripts/fetch_dawn.sh first."
  exit 1
fi

if [ -d "$EMSDK" ]; then
  . "$EMSDK/emsdk_env.sh"
fi

mkdir -p "$BUILD_DIR"

cmake -S "$DAWN_DIR" -B "$BUILD_DIR" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DDAWN_BUILD_SAMPLES=OFF \
  -DDAWN_BUILD_TESTS=OFF \
  -DDAWN_ENABLE_DESKTOP_GL=OFF \
  -DDAWN_ENABLE_OPENGLES=OFF \
  -DDAWN_ENABLE_VULKAN=OFF \
  -DDAWN_ENABLE_METAL=OFF \
  -DDAWN_ENABLE_D3D11=OFF \
  -DDAWN_ENABLE_D3D12=OFF \
  -DDAWN_ENABLE_NULL=OFF \
  -DDAWN_ENABLE_EMSCRIPTEN=ON

cmake --build "$BUILD_DIR"

echo "Dawn web build complete at $BUILD_DIR"
