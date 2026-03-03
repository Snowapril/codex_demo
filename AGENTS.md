# Rendering Engine Plan (Reference)

## Summary
- C++20 core + Objective‑C++ Metal backend.
- macOS (Apple Silicon only): Metal4 + Vulkan (via KosmicKrisp) + WebGPU (Dawn), iOS: Metal4 only, Windows: Vulkan + WebGPU (Dawn), Web: WebGPU (Dawn) via WASM in Chrome.
- HLSL runtime compile via DXC: SPIR‑V for Vulkan, DXIL → Metal Shader Converter → metallib for Metal.
- RenderGraph flow: **Build → Compile → Resolve → Execute**. Command buffers are created/recorded/executed from the graph.
- Resource tracking and dependency analysis determine queue compatibility and required sync (auto‑inserted).
- CI runs validation layers and headless smoke tests on Windows/macOS.

## 1) Architecture & Repo Layout
- `/engine/include` public API
- `/engine/src` core (RHI, resource, render graph, scheduler)
- `/engine/src/backends/vulkan`
- `/engine/src/backends/metal` (Objective‑C++ `.mm`)
- `/engine/src/backends/webgpu` (Dawn)
- `/web` WASM + Chrome launcher/support
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
- WebGPU (Dawn/WASM): HLSL → WGSL or SPIR‑V → WGSL via Dawn tooling (TBD per build target)

## 6) Backend Details
- Vulkan backend
  - Enable `VK_LAYER_KHRONOS_validation` in Debug/CI
- Metal backend
  - Enable `MTL_DEBUG_LAYER=1`, `MTL_SHADER_VALIDATION=1`
- WebGPU backend (Dawn)
  - Use Dawn validation toggles in Debug/CI
  - Support WASM builds for Chrome
- ML pass
  - Metal4 only; excluded with warning on Vulkan/WebGPU

## 7) Build System (CMake)
- `RENG_ENABLE_METAL`
- `RENG_ENABLE_VULKAN`
- `RENG_ENABLE_VALIDATION`
- `RENG_ENABLE_ML_PASS`
- `RENG_ENABLE_WEBGPU`
- `RENG_ENABLE_WEB_WASM`

### 7.1 WebGPU (Dawn) Build Notes
- Desktop: build Dawn as a dependency and link against native Dawn (D3D12/Metal/Vulkan).
- Web: build Dawn in WASM mode via Emscripten; produce a `.wasm` + JS loader.
- Provide a simple web launcher under `/web` that hosts the WASM build and requests a WebGPU device.

## 8) CI (GitHub Actions)
- Windows: Vulkan SDK + DXC + Dawn, build + headless tests
- macOS: Vulkan SDK + Metal Shader Converter + Dawn, Metal/Vulkan/WebGPU headless tests
- Web (Chrome): WASM build + WebGPU smoke test

### 8.1 Web CI Details
- Install Emscripten SDK (emsdk) and activate a fixed version.
- Build Dawn for web target; then build the engine with `RENG_ENABLE_WEBGPU` + `RENG_ENABLE_WEB_WASM`.
- Package `/web` artifacts and run a minimal smoke test:
  - Launch a local HTTP server.
  - Run a headless Chrome test that requests WebGPU and renders a single frame.

## 9) Tests & Scenarios
- RenderGraph compile test:
  - Pass resource conflicts detected
  - Auto sync insertion logged
- Queue scheduling test:
  - Transfer pass separated from Graphics pass where possible
- ML pass test (macOS only):
  - ML pass enabled only on Metal

## 10) Assumptions & Defaults
- Minimum OS: macOS 26 / iOS 26 / Windows 11
- macOS: Apple Silicon only
- iOS: Metal4 only
- RenderGraph compile/resolve required each frame
- Auto‑sync insertion enabled
- macOS/iOS frame pacing uses CAMetalDisplayLink
