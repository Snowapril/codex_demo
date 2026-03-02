#!/usr/bin/env python3
import os
import subprocess


def main():
  build_dir = os.environ.get("BUILD_DIR", "build")
  subprocess.check_call(["ctest", "--test-dir", build_dir, "--output-on-failure"])
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
