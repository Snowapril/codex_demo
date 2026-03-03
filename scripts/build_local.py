#!/usr/bin/env python3
import os
import subprocess
import sys


def run(cmd):
  print("+", " ".join(cmd))
  subprocess.check_call(cmd)


def main():
  build_dir = os.environ.get("BUILD_DIR", "build")
  backend = sys.argv[1] if len(sys.argv) > 1 else "all"

  flags = {
      "vulkan": ["-DRENG_ENABLE_VULKAN=ON", "-DRENG_ENABLE_METAL=OFF", "-DRENG_ENABLE_WEBGPU=OFF"],
      "metal": ["-DRENG_ENABLE_VULKAN=OFF", "-DRENG_ENABLE_METAL=ON", "-DRENG_ENABLE_WEBGPU=OFF"],
      "webgpu": ["-DRENG_ENABLE_VULKAN=OFF", "-DRENG_ENABLE_METAL=OFF", "-DRENG_ENABLE_WEBGPU=ON"],
      "all": ["-DRENG_ENABLE_VULKAN=ON", "-DRENG_ENABLE_METAL=ON", "-DRENG_ENABLE_WEBGPU=ON"],
  }

  if backend not in flags:
    print("Usage: scripts/build_local.py [vulkan|metal|webgpu|all]")
    return 1

  run(["cmake", "-S", ".", "-B", build_dir] + flags[backend])
  run(["cmake", "--build", build_dir, "--config", "Release"])
  return 0


if __name__ == "__main__":
  sys.exit(main())
