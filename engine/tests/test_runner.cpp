#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "reng/logger.h"

namespace {
std::filesystem::path exeDir(const char* argv0) {
  std::filesystem::path path(argv0 ? argv0 : "");
  if (path.has_parent_path()) {
    return path.parent_path();
  }
  return std::filesystem::current_path();
}

int runChild(const std::filesystem::path& dir, const std::string& name) {
  std::filesystem::path exe = dir / name;
#if defined(_WIN32)
  std::string cmd = "\"" + exe.string() + "\"";
#else
  std::string cmd = exe.string();
#endif
  return std::system(cmd.c_str());
}
}  // namespace

int main(int argc, char** argv) {
  using namespace reng;
  std::filesystem::create_directories("logs");
  RengLogger::init("logs/tests_runner.log");
  RengLogger::logInfo("Starting test runner");

  std::filesystem::path dir = exeDir(argc > 0 ? argv[0] : nullptr);
  bool isHeadless = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i] ? argv[i] : "";
    if (arg == "--headless") {
      isHeadless = true;
    }
  }
  std::vector<std::string> tests = {"reng_render_graph_tests"};
  if (isHeadless) {
    RengLogger::logWarning(
        "Headless mode enabled; skipping swapchain test");
  } else {
    tests.push_back("reng_swapchain_smoke_tests");
  }

  int failed = 0;
  for (const auto& test : tests) {
    RengLogger::logInfo("Running test {}", test);
    int code = runChild(dir, test);
    if (code == 0) {
      RengLogger::logInfo("Test {} passed", test);
    } else {
      RengLogger::logError("Test {} failed with code {}", test, code);
      ++failed;
    }
  }

  if (failed == 0) {
    RengLogger::logInfo("All tests passed");
  } else {
    RengLogger::logError("{} tests failed", failed);
  }

  RengLogger::shutdown();
  return failed == 0 ? 0 : 1;
}
