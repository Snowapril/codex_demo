cmake_minimum_required(VERSION 3.20)
project(reng LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(RENG_ENABLE_METAL "Enable Metal backend" ON)
option(RENG_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(RENG_ENABLE_VALIDATION "Enable validation layers" ON)
option(RENG_ENABLE_ML_PASS "Enable Metal ML pass" ON)

add_subdirectory(engine)
add_subdirectory(samples/triangle)
add_subdirectory(samples/blank)

enable_testing()
add_subdirectory(engine/tests)
