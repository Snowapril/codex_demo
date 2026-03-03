#include "reng/app.h"

namespace reng {

int runAppPlatform(const AppDesc& desc, AppCallbacks& callbacks);

int runApp(const AppDesc& desc, AppCallbacks& callbacks) {
  return runAppPlatform(desc, callbacks);
}

}  // namespace reng
