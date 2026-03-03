#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <string>

#include "reng/app.h"
#include "reng/engine.h"
#include "reng/logger.h"
#include "reng/platform.h"

namespace {
const uint32_t kDefaultWidth = 800;
const uint32_t kDefaultHeight = 600;

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

std::wstring toWide(const char* utf8) {
  if (!utf8 || *utf8 == '\0') {
    return L"Blank Sample";
  }
  int size =
      MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
  if (size <= 0) {
    return L"Blank Sample";
  }
  std::wstring result(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.data(), size);
  if (!result.empty() && result.back() == L'\0') {
    result.pop_back();
  }
  return result;
}
}  // namespace

namespace reng {

int runAppPlatform(const AppDesc& desc, AppCallbacks& callbacks) {
  if (desc.backend != Backend::Vulkan) {
    return 1;
  }
  RengLogger::logInfo("Starting Windows app");
  const uint32_t width =
      desc.swapchain.width ? desc.swapchain.width : kDefaultWidth;
  const uint32_t height =
      desc.swapchain.height ? desc.swapchain.height : kDefaultHeight;

  const wchar_t kClassName[] = L"BlankVulkanWindow";
  WNDCLASS wc{};
  wc.lpfnWndProc = windowProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = kClassName;
  RegisterClass(&wc);

  std::wstring title = toWide(desc.title);
  HWND hwnd = CreateWindowEx(0, kClassName, title.c_str(),
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             static_cast<int>(width), static_cast<int>(height),
                             nullptr, nullptr, wc.hInstance, nullptr);
  if (!hwnd) {
    return 1;
  }

  ShowWindow(hwnd, SW_SHOW);

  PlatformContext context;
  context.platform = PlatformKind::Windows;
  context.windows.hinstance = wc.hInstance;
  context.windows.hwnd = hwnd;
  auto engine = Engine::create(desc, callbacks, context);
  if (!engine) {
    DestroyWindow(hwnd);
    return 1;
  }

  ULONGLONG lastTick = GetTickCount64();
  ULONGLONG startTick = lastTick;
  MSG msg{};
  while (msg.message != WM_QUIT) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    ULONGLONG now = GetTickCount64();
    float delta = static_cast<float>(now - lastTick) / 1000.0f;
    lastTick = now;

    engine->tick(delta);
    if (callbacks.shouldExit()) {
      PostQuitMessage(0);
    }
    if (desc.maxRunSeconds > 0.0f) {
      ULONGLONG elapsed = now - startTick;
      if (elapsed >= static_cast<ULONGLONG>(desc.maxRunSeconds * 1000.0f)) {
        PostQuitMessage(0);
      }
    }
  }

  DestroyWindow(hwnd);
  RengLogger::logInfo("Shutting down Windows app");
  RengLogger::shutdown();
  return 0;
}

}  // namespace reng
