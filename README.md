# Render Graph Engine

![CI](https://github.com/Snowapril/codex_demo/actions/workflows/ci.yml/badge.svg?branch=main)

Cross‑platform rendering engine skeleton with a render‑graph‑first API. Targets:

- macOS (Apple Silicon only): Metal4 + Vulkan (via KosmicKrisp) + WebGPU (Dawn)
- iOS: Metal4 only
- Windows: Vulkan + WebGPU (Dawn)
- Web: WebGPU (Dawn) via WASM in Chrome

## Goals

- HLSL runtime compile via DXC: SPIR‑V for Vulkan, DXIL → Metal Shader Converter → metallib for Metal
- WebGPU backend uses Google Dawn, including WASM builds for Chrome
- RenderGraph flow: Build → Compile → Resolve → Execute
- Resource tracking + dependency analysis for queue scheduling and autoSync insertion
- Validation layers enabled in CI and local testing

## RenderGraph API (MVP)

- `RenderGraph::beginFrame()`
- `RenderGraph::addBlitPass(...)`
- `RenderGraph::addRenderPass(..., framebufferName, ...)`
- `RenderGraph::addComputePass(...)`
- `RenderGraph::addMLPass(...)` (Metal only)
- `RenderGraph::compile(CompileOptions)`
- `RenderGraph::resolve(Device&) -> ResolvedFrame`
- `ResolvedFrame::execute()`

## Build

```sh
cmake -S . -B build -DRENG_ENABLE_VALIDATION=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Local Backend Builds

One‑touch build scripts are available under `scripts/`:

- Build a specific backend:
  - `./scripts/build_local.py vulkan`
  - `./scripts/build_local.py metal`
  - `./scripts/build_local.py webgpu`
- Build all backends:
  - `./scripts/build_local.py all`

### WebGPU (WASM) Local Test

Run a local WebGPU smoke test (Chrome required):

```sh
./scripts/test_web_local.py
```

### Local Tests

```sh
./scripts/test_local.py
```

## Web (WASM) Build (Plan)

- Build Dawn for Web (Emscripten) and enable `RENG_ENABLE_WEBGPU` + `RENG_ENABLE_WEB_WASM`.
- Produce `.wasm` + JS loader artifacts under `/web`.
- Run a minimal WebGPU smoke test in Chrome (one frame render).

## Formatting

- clang‑format is enforced via pre‑commit hook (`.githooks/pre-commit`)
- Third‑party code under `third_party/` is excluded from formatting

## Repository Layout

- `engine/include` public API
- `engine/src` core implementation + backends
- `samples/triangle` minimal sample
- `.github/workflows` CI

## License

MIT
