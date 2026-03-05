# Render Graph Engine

![CI](https://github.com/Snowapril/codex_demo/actions/workflows/ci.yml/badge.svg?branch=main)

Cross‑platform rendering engine skeleton with a render‑graph‑first API. Targets:

- macOS (Apple Silicon only): Metal4 + Vulkan (via KosmicKrisp)
- iOS: Metal4 only
- Windows: Vulkan only

## Goals

- HLSL runtime compile via DXC: SPIR‑V for Vulkan, DXIL → Metal Shader Converter → metallib for Metal
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
cmake -S . -B build -DRENG_ENABLE_VALIDATION=ON -DRENG_BUILD_SAMPLES=ON -DRENG_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

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
