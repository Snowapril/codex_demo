#!/usr/bin/env python3
import http.server
import os
import socketserver
import subprocess
import threading
import time


def serve_web():
  os.chdir("web")
  with socketserver.TCPServer(("", 8000), http.server.SimpleHTTPRequestHandler) as httpd:
    httpd.serve_forever()


def main():
  thread = threading.Thread(target=serve_web, daemon=True)
  thread.start()
  time.sleep(1)

  subprocess.check_call(["node", "web/smoke_test.js"])
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
