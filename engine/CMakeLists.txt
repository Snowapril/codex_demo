add_library(reng_core
  src/core/render_graph.cpp
  src/core/resources.cpp
  src/core/command_buffer.cpp
  src/core/device.cpp
)

target_include_directories(reng_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(RENG_ENABLE_VULKAN)
  add_library(reng_backend_vulkan src/backends/vulkan/vulkan_backend.cpp)
  target_link_libraries(reng_backend_vulkan PUBLIC reng_core)
  target_compile_definitions(reng_backend_vulkan PUBLIC RENG_ENABLE_VULKAN=1)
endif()

if(RENG_ENABLE_WEBGPU)
  add_library(reng_backend_webgpu src/backends/webgpu/webgpu_backend.cpp)
  target_link_libraries(reng_backend_webgpu PUBLIC reng_core)
  target_compile_definitions(reng_backend_webgpu PUBLIC RENG_ENABLE_WEBGPU=1)
endif()

if(APPLE AND RENG_ENABLE_METAL)
  add_library(reng_backend_metal src/backends/metal/metal_backend.mm)
  target_link_libraries(reng_backend_metal PUBLIC reng_core)
  target_compile_definitions(reng_backend_metal PUBLIC RENG_ENABLE_METAL=1)
  set_source_files_properties(src/backends/metal/metal_backend.mm PROPERTIES
    LANGUAGE OBJCXX
  )
endif()
