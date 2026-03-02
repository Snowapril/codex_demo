cmake_minimum_required(VERSION 3.20)
project(reng LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(RENG_ENABLE_METAL "Enable Metal backend" ON)
option(RENG_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(RENG_ENABLE_VALIDATION "Enable validation layers" ON)
option(RENG_ENABLE_ML_PASS "Enable Metal ML pass" ON)
option(RENG_ENABLE_WEBGPU "Enable WebGPU backend" ON)
option(RENG_ENABLE_WEB_WASM "Enable Web WASM build" OFF)

add_subdirectory(engine)
add_subdirectory(samples/triangle)

if(RENG_ENABLE_WEB_WASM)
  add_subdirectory(web)
endif()

enable_testing()
add_subdirectory(engine/tests)
