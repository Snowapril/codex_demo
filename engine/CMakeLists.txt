add_library(reng_core
  src/core/render_graph.cpp
  src/core/resources.cpp
  src/core/command_buffer.cpp
  src/core/device.cpp
  src/platform/app.cpp
)

target_include_directories(reng_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(RENG_ENABLE_VULKAN)
  add_library(reng_backend_vulkan src/backends/vulkan/vulkan_backend.cpp)
  target_link_libraries(reng_backend_vulkan PUBLIC reng_core)
  target_compile_definitions(reng_backend_vulkan PUBLIC RENG_ENABLE_VULKAN=1)
endif()

if(APPLE AND RENG_ENABLE_METAL)
  add_library(reng_backend_metal src/backends/metal/metal_backend.mm)
  target_link_libraries(reng_backend_metal PUBLIC reng_core)
  target_compile_definitions(reng_backend_metal PUBLIC RENG_ENABLE_METAL=1)
  set_source_files_properties(src/backends/metal/metal_backend.mm PROPERTIES
    LANGUAGE OBJCXX
  )
endif()

if(APPLE)
  target_sources(reng_core PRIVATE src/platform/macos_app.mm)
  target_link_libraries(reng_core PRIVATE \"-framework Cocoa\" \"-framework Metal\" \"-framework QuartzCore\")
  set_source_files_properties(src/platform/macos_app.mm PROPERTIES
    LANGUAGE OBJCXX
  )
endif()

if(WIN32)
  target_sources(reng_core PRIVATE src/platform/windows_app.cpp)
  find_package(Vulkan REQUIRED)
  target_link_libraries(reng_core PRIVATE Vulkan::Vulkan)
endif()
