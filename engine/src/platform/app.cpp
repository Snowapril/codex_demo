#include "reng/app.h"

#include <string>

namespace reng {

int runAppPlatform(const AppDesc& desc, AppCallbacks& callbacks);

namespace {

AppDesc applyCommandLine(AppDesc desc, int argc, char** argv) {
  if (argc <= 1 || !argv) {
    return desc;
  }

  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--backend" || arg == "-b") {
      if (i + 1 < argc) {
        arg = argv[++i];
      } else {
        continue;
      }
    } else if (arg.rfind("--backend=", 0) == 0) {
      arg = arg.substr(std::string("--backend=").size());
    } else if (arg == "--validation") {
      desc.device.enableValidation = true;
      continue;
    } else if (arg == "--no-validation") {
      desc.device.enableValidation = false;
      continue;
    } else if (arg.rfind("--validation=", 0) == 0) {
      std::string value = arg.substr(std::string("--validation=").size());
      desc.device.enableValidation =
          (value == "1" || value == "true" || value == "on");
      continue;
    } else {
      continue;
    }

    if (arg == "metal") {
      desc.backend = Backend::Metal;
    } else if (arg == "vulkan") {
      desc.backend = Backend::Vulkan;
    }
  }

  return desc;
}

}  // namespace

int runApp(const AppDesc& desc, AppCallbacks& callbacks) {
  return runAppPlatform(desc, callbacks);
}

int runApp(int argc, char** argv, const AppDesc& desc,
           AppCallbacks& callbacks) {
  return runAppPlatform(applyCommandLine(desc, argc, argv), callbacks);
}

}  // namespace reng
