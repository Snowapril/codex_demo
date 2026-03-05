#pragma once

namespace reng {

enum class PlatformKind {
  MacOS,
  Windows,
};

struct MacOSContext {
  void* nsWindow = nullptr;
  void* metalLayer = nullptr;
};

struct WindowsContext {
  void* hinstance = nullptr;
  void* hwnd = nullptr;
};

struct PlatformContext {
  PlatformKind platform = PlatformKind::MacOS;
  MacOSContext macos;
  WindowsContext windows;
};

}  // namespace reng
