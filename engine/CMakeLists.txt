add_library(reng_core
  src/core/render_graph.cpp
  src/core/resources.cpp
  src/core/command_buffer.cpp
  src/core/device.cpp
  src/core/engine.cpp
  src/core/logger.cpp
  src/core/resource_pool.cpp
  src/platform/app.cpp
)

target_include_directories(reng_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glm
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(RENG_ENABLE_VULKAN)
  add_library(reng_backend_vulkan src/backends/vulkan/vulkan_backend.cpp)
  target_link_libraries(reng_backend_vulkan PUBLIC reng_core)
  target_compile_definitions(reng_backend_vulkan PUBLIC RENG_ENABLE_VULKAN=1)
  target_compile_definitions(reng_core PUBLIC RENG_ENABLE_VULKAN=1)
endif()

if(RENG_ENABLE_VALIDATION)
  target_compile_definitions(reng_core PUBLIC RENG_ENABLE_VALIDATION=1)
endif()

if(APPLE AND RENG_ENABLE_METAL)
  add_library(reng_backend_metal src/backends/metal/metal_backend.mm)
  target_link_libraries(reng_backend_metal PUBLIC reng_core)
  target_compile_definitions(reng_backend_metal PUBLIC RENG_ENABLE_METAL=1)
  target_compile_definitions(reng_core PUBLIC RENG_ENABLE_METAL=1)
  set_source_files_properties(src/backends/metal/metal_backend.mm PROPERTIES
    LANGUAGE OBJCXX
  )
endif()

if(APPLE)
  target_sources(reng_core PRIVATE
    src/platform/macos_app.mm
    src/platform/backend_factory_macos.mm
    src/backends/metal/metal_command_queue.mm
    src/backends/metal/metal_command_buffer.mm
    src/backends/metal/metal_device.mm
    src/backends/metal/metal_queue_timeline.mm
    src/backends/metal/metal_resources.mm
    src/backends/metal/metal_swapchain.mm
    src/backends/metal/metal_utils.mm
  )
  target_link_libraries(reng_core PUBLIC
    "-framework Cocoa"
    "-framework Metal"
    "-framework QuartzCore"
  )
  set_source_files_properties(
    src/platform/macos_app.mm
    src/backends/metal/metal_device.mm
    src/backends/metal/metal_swapchain.mm
    PROPERTIES LANGUAGE OBJCXX
  )
endif()

if(APPLE AND RENG_ENABLE_VULKAN)
  target_sources(reng_core PRIVATE
    src/backends/vulkan/vulkan_command_buffer.cpp
    src/backends/vulkan/vulkan_command_queue.cpp
    src/backends/vulkan/vulkan_device.cpp
    src/backends/vulkan/vulkan_queue_timeline.cpp
    src/backends/vulkan/vulkan_resources.cpp
    src/backends/vulkan/vulkan_swapchain.cpp
    src/backends/vulkan/vulkan_utils.cpp
  )
  find_package(Vulkan REQUIRED)
  target_link_libraries(reng_core PRIVATE Vulkan::Vulkan)
endif()

if(WIN32)
  target_sources(reng_core PRIVATE
    src/platform/windows_app.cpp
    src/platform/backend_factory_windows.cpp
    src/backends/vulkan/vulkan_command_buffer.cpp
    src/backends/vulkan/vulkan_command_queue.cpp
    src/backends/vulkan/vulkan_device.cpp
    src/backends/vulkan/vulkan_queue_timeline.cpp
    src/backends/vulkan/vulkan_resources.cpp
    src/backends/vulkan/vulkan_swapchain.cpp
    src/backends/vulkan/vulkan_utils.cpp
  )
  find_package(Vulkan REQUIRED)
  target_link_libraries(reng_core PRIVATE Vulkan::Vulkan)
endif()
