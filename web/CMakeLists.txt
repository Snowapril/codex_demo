cmake_minimum_required(VERSION 3.20)

project(reng_web LANGUAGES CXX)

# Placeholder target. Real engine WASM target will be added later.
add_custom_target(reng_web_wasm ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/index.html
          ${CMAKE_CURRENT_SOURCE_DIR}/app.js
          ${CMAKE_CURRENT_BINARY_DIR}/dist
  BYPRODUCTS
          ${CMAKE_CURRENT_BINARY_DIR}/dist/index.html
          ${CMAKE_CURRENT_BINARY_DIR}/dist/app.js
  COMMENT "Preparing web assets"
)
