#!/usr/bin/env python3
import os
import subprocess
import sys


def run(cmd, cwd=None, env=None):
  print("+", " ".join(cmd))
  subprocess.check_call(cmd, cwd=cwd, env=env)


def main():
  dawn_dir = os.environ.get("DAWN_DIR", "third_party/dawn")
  build_dir = os.environ.get("BUILD_DIR", "third_party/dawn_out/web")
  emsdk = os.environ.get("EMSDK", os.path.expanduser("~/emsdk"))

  if not os.path.isdir(dawn_dir):
    print(f"Dawn not found at {dawn_dir}. Run scripts/fetch_dawn.sh first.")
    return 1

  env = os.environ.copy()
  emsdk_env = os.path.join(emsdk, "emsdk_env.sh")
  if os.path.isfile(emsdk_env):
    # Load emsdk env in a subshell and capture the environment.
    dump_cmd = ["/bin/sh", "-lc", f". {emsdk_env} >/dev/null 2>&1 && env"]
    output = subprocess.check_output(dump_cmd, text=True)
    for line in output.splitlines():
      if "=" in line:
        k, v = line.split("=", 1)
        env[k] = v

  if os.path.isdir(build_dir):
    print(f"Removing existing build dir: {build_dir}")
    subprocess.check_call(["rm", "-rf", build_dir])
  os.makedirs(build_dir, exist_ok=True)

  emcmake = "emcmake"
  emsdk_emcmake = os.path.join(emsdk, "upstream", "emscripten", "emcmake")
  if os.path.isfile(emsdk_emcmake):
    emcmake = emsdk_emcmake

  toolchain_path = os.path.join(emsdk, "upstream", "emscripten")

  run([
      emcmake,
      "cmake",
      "-S",
      dawn_dir,
      "-B",
      build_dir,
      "-GNinja",
      "-DCMAKE_BUILD_TYPE=Release",
      f"-DDAWN_EMSCRIPTEN_TOOLCHAIN={toolchain_path}",
      "-DDAWN_BUILD_SAMPLES=OFF",
      "-DDAWN_BUILD_TESTS=OFF",
      "-DDAWN_FETCH_DEPENDENCIES=ON",
      "-DDAWN_ENABLE_DESKTOP_GL=OFF",
      "-DDAWN_ENABLE_OPENGLES=OFF",
      "-DDAWN_ENABLE_VULKAN=OFF",
      "-DDAWN_ENABLE_METAL=OFF",
      "-DDAWN_ENABLE_D3D11=OFF",
      "-DDAWN_ENABLE_D3D12=OFF",
      "-DDAWN_ENABLE_NULL=OFF",
      "-DDAWN_ENABLE_GLFW=OFF",
      "-DDAWN_ENABLE_X11=OFF",
      "-DGLFW_BUILD_X11=OFF",
      "-DGLFW_BUILD_WAYLAND=OFF",
      "-DDAWN_ENABLE_EMSCRIPTEN=ON",
      "-DTINT_BUILD_TESTS=OFF",
      "-DTINT_BUILD_CMD_TOOLS=OFF",
      "-DTINT_BUILD_IR_BINARY=OFF",
  ], env=env)

  run(["cmake", "--build", build_dir], env=env)

  print(f"Dawn web build complete at {build_dir}")
  return 0


if __name__ == "__main__":
  sys.exit(main())
