# Render Graph Engine

Cross‑platform rendering engine skeleton with a render‑graph‑first API. Targets:

- macOS (Apple Silicon only): Metal4 + Vulkan (via KosmicKrisp)
- iOS: Metal4 only
- Windows: Vulkan only

## Goals

- HLSL runtime compile via DXC: SPIR‑V for Vulkan, DXIL → Metal Shader Converter → metallib for Metal
- Render Graph flow: Build → Compile → Resolve → Execute
- Resource tracking + dependency analysis for queue scheduling and auto‑sync insertion
- Validation layers enabled in CI and local testing

## Render Graph API (MVP)

- `RenderGraph::BeginFrame()`
- `RenderGraph::AddBlitPass(...)`
- `RenderGraph::AddRenderPass(..., framebufferName, ...)`
- `RenderGraph::AddComputePass(...)`
- `RenderGraph::AddMLPass(...)` (Metal only)
- `RenderGraph::Compile(CompileOptions)`
- `RenderGraph::Resolve(Device&) -> ResolvedFrame`
- `ResolvedFrame::Execute()`

## Build

```sh
cmake -S . -B build -DRENG_ENABLE_VALIDATION=ON
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
