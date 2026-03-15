# Rendering Engine Plan (Reference)

## Summary
- C++20 core + Objective‑C++ Metal backend (Metal4 APIs, e.g. `MTL4CommandQueue`).
- macOS (Apple Silicon only): Metal4 + Vulkan (via KosmicKrisp), iOS: Metal4 only, Windows: Vulkan only.
- HLSL runtime compile via DXC: SPIR‑V for Vulkan, DXIL → Metal Shader Converter → metallib for Metal.
- RenderGraph flow: **Build → Compile → Resolve → Execute**. Command buffers are created/recorded/executed from the graph.
- Resource tracking and dependency analysis determine queue compatibility and required sync (auto‑inserted).
- CI runs validation layers and headless smoke tests on Windows/macOS.
  - macOS Metal tests run with `MTL_DEBUG_LAYER=1` set in the invocation environment.

## 1) Architecture & Repo Layout
- `/engine/include` public API
- `/engine/src` core (RHI, resource, render graph, scheduler)
- `/engine/src/backends/vulkan`
- `/engine/src/backends/metal` (Objective‑C++ `.mm`)
- `/samples/triangle` minimal sample (platform entrypoints)
- `/tools/shader_cache`
- `/cmake`
- `/.github/workflows`

## 2) RenderGraph‑First API

### 2.1 Lifecycle
1. **Build:** Register all passes required for the frame
2. **Compile:** Analyze resource usage (READ/WRITE/STATE), build dependency graph, and compute queue schedule
3. **Resolve:** Allocate/record actual API command buffers from the schedule
4. **Execute:** Submit resolved command buffers to queues

### 2.2 User Pass Shapes
- Blit pass:
  - `beginBlitPass`, `copyTexture`, `uploadBuffer`, `uploadTexture`, `endBlitPass`
- Render pass:
  - `beginRenderPass(framebuffer)`, `draw`, `endRenderPass`
- Compute pass:
  - `beginComputePass`, `dispatch`, `endComputePass`
- ML pass (Metal4 only):
  - `beginMLPass`, `dispatch`, `endMLPass`

### 2.3 Resource Tracking Model
- Each pass declares resources and access:
  - Buffer/Texture: `READ`, `WRITE`, `READ_WRITE`
  - Textures also declare `Usage` and `Layout/State` (RenderTarget, Sampled, Storage, etc.)
- RenderGraph collects and stores the access list per pass

### 2.4 Dependency Analysis & Scheduling
- Compile step detects RAW/WAR/WAW hazards
- Determines whether passes can run concurrently across different queues
- Produces a Pass Scheduling Plan:
  - Pass → Queue mapping
  - Required sync (barriers, semaphores/fences) auto‑inserted

### 2.5 Auto Sync Policy
- If hazards are detected:
  - Auto‑insert synchronization where possible
  - Same behavior in Debug/Release
  - Sync insertion is logged/visible in compile report

## 3) Public API (MVP)

### 3.1 RenderGraph API
- `RenderGraph::beginFrame()`
- `RenderGraph::addBlitPass(...)`
- `RenderGraph::addRenderPass(..., framebufferName, ...)`
- `RenderGraph::addComputePass(...)`
- `RenderGraph::addMLPass(...)` (Metal only)
- `RenderGraph::compile(CompileOptions)`
- `RenderGraph::resolve(Device&) -> ResolvedFrame`
- `ResolvedFrame::execute()`

### 3.2 Dependency Visibility
- `RenderGraph::compile()` returns report with:
  - Per‑pass queue assignment
  - Dependency edges between passes
  - Auto‑inserted sync list
  - Concurrency eligibility flags

## 4) Queue Model
- Three queue types: Graphics / Compute / Transfer
- Each pass may declare preferred queue type
- RenderGraph decides final mapping

## 5) Shader Pipeline (Runtime Compile)
- Hash‑based cache
- Vulkan: DXC SPIR‑V generation
- Metal: DXC → DXIL → Metal Shader Converter → metallib

## 6) Backend Details
- Vulkan backend
  - Enable `VK_LAYER_KHRONOS_validation` in Debug/CI
- Metal backend
  - Enable `MTL_DEBUG_LAYER=1`, `MTL_SHADER_VALIDATION=1`
- ML pass
  - Metal4 only; excluded with warning on Vulkan

## 7) Build System (CMake)
- `RENG_ENABLE_METAL`
- `RENG_ENABLE_VULKAN`
- `RENG_ENABLE_VALIDATION`
- `RENG_ENABLE_ML_PASS`
- `RENG_BUILD_SAMPLES` (default ON)
- `RENG_BUILD_TESTS` (default ON)
- `RENG_HEADLESS_TESTS` (ctest uses `--headless` when ON)

## 8) CI (GitHub Actions)
- Windows: Vulkan SDK + DXC, build + headless tests
- macOS: Vulkan SDK 1.4.341.1 + Metal Shader Converter, Metal/Vulkan headless tests

## Vulkan SDK Minimum
- Windows/macOS minimum Vulkan SDK: 1.4.341.1

## Vulkan Specification
- Vulkan backend code follows the latest Vulkan specification:
  - `https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html`

## 9) Tests & Scenarios
- RenderGraph compile test:
  - Pass resource conflicts detected
  - Auto sync insertion logged
- Swapchain smoke test:
  - Blank screen for a few frames (skipped in headless)

## 10) Assumptions & Defaults
- Minimum OS: macOS 26 / iOS 26 / Windows 11
- macOS: Apple Silicon only
- iOS: Metal4 only
- RenderGraph compile/resolve required each frame
- Auto‑sync insertion enabled
- macOS/iOS frame pacing uses CAMetalDisplayLink

## 11) Engine & Platform Rules
- App code must not include backend headers; Engine selects backend by `AppDesc.backend`.
- Backend creation is done by platform factories; app only provides `PlatformContext`.
- Swapchain creation is backend‑owned; app provides width/height/pixel format and present mode.
- macOS Vulkan uses KosmicKrisp: do not enable portability enumeration.
- Runloop is centralized in Engine; callbacks: input → update frame → update render → render.
- Engine parses command line args for backend/validation (for example `--backend=metal`, `--backend=vulkan`, `--validation`) so all executables share the same switch.

## 12) Logging
- Use `RengLogger` (no `std::cerr`).
- Log format: `[yyyy-mm-dd-hh-mm-ss] [LogLevel] message`.
- Logger initialized by samples; shutdown on app exit.

## 13) Naming
- Methods/functions: `camelCase`.
- File names keep current style (no camelCase rename).
- Member naming: `_{name}` for private/protected, `{name}` for public.

## 14) Pre‑Push Verification
- Before pushing:
  - On macOS: run tests for both Metal and Vulkan backends.
  - On Windows: run tests for the Vulkan backend.
  - Confirm both build and tests pass.
