# Decisions

## Architecture
- RenderGraph‑first design: Build → Compile → Resolve → Execute.
- Command buffers are created/recorded/executed from the RenderGraph.
- Resource tracking + dependency analysis decide queue compatibility and auto‑sync.

## Backends & Platforms
- Backends: Metal4 + Vulkan.
- macOS: Metal4 + Vulkan (via KosmicKrisp emulation layer).
- iOS: Metal4 only.
- Windows: Vulkan only.
- macOS Vulkan must **not** use portability enumeration (KosmicKrisp requirement).

## Engine/Platform Boundary
- App code must **not** include backend headers.
- App supplies `PlatformContext` and `AppDesc.backend`; Engine creates backend internally.
- Swapchain is backend‑owned; app provides width/height/pixel format/present mode.
- Runloop is centralized in `Engine`:
  - `onInput` → `onUpdateFrame` → `onUpdateRender` → `onRender`.
- macOS uses `CAMetalDisplayLink` (fallback timer if unavailable).

## Swapchain
- Swapchain supports `presentMode` and `recreate(...)`.
- Present modes: Vsync / Immediate.

## Logging
- Use `RengLogger` everywhere (no `std::cerr`).
- Log format: `[yyyy-mm-dd-hh-mm-ss] [LogLevel] message`.
- Samples initialize logger and choose log file path.
- Logger must be shut down on app exit.

## Tests
- `reng_test_runner` runs all tests and logs pass/fail.
- Headless mode uses CLI flag `--headless` (CMake option `RENG_HEADLESS_TESTS`).
- Swapchain smoke test is skipped in headless mode.

## Build & CI
- CMake options:
  - `RENG_ENABLE_METAL`
  - `RENG_ENABLE_VULKAN`
  - `RENG_ENABLE_VALIDATION`
  - `RENG_ENABLE_ML_PASS`
  - `RENG_BUILD_SAMPLES`
  - `RENG_BUILD_TESTS`
  - `RENG_HEADLESS_TESTS`
- CI runs tests after build on Windows/macOS.
- Validation layers enabled in CI and local testing.
- CI uses Vulkan SDK 1.4.341.1 on macOS.

## Vulkan SDK Minimum
- Windows/macOS minimum Vulkan SDK: 1.4.341.1

## Pre‑Push Verification
- Before pushing:
  - macOS: run tests for both Metal and Vulkan backends.
  - Windows: run tests for the Vulkan backend.
  - Confirm build and tests pass.

## Naming
- Methods/functions: `camelCase`.
- File names keep existing style (do not rename to camelCase).
- Members: private/protected `_{name}`, public `{name}`.
