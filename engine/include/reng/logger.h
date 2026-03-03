#pragma once

#include <format>
#include <string>
#include <string_view>

namespace reng {

class RengLogger {
 public:
  enum class Level {
    Verbose,
    Info,
    Warning,
    Error,
  };

  static bool init(const std::string& filePath);
  static void shutdown();

  static void log(Level level, std::string_view message);

  template <typename... Args>
  static void logVerbose(std::string_view format, Args&&... args) {
    log(Level::Verbose,
        formatMessage(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void logInfo(std::string_view format, Args&&... args) {
    log(Level::Info,
        formatMessage(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void logWarning(std::string_view format, Args&&... args) {
    log(Level::Warning,
        formatMessage(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void logError(std::string_view format, Args&&... args) {
    log(Level::Error,
        formatMessage(format, std::forward<Args>(args)...));
  }

 private:
 static std::string formatMessage(std::string_view format);

  template <typename... Args>
  static std::string formatMessage(std::string_view format, Args&&... args) {
    return formatMessageImpl(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static std::string formatMessageImpl(std::string_view format,
                                       Args&&... args) {
    return std::vformat(format, std::make_format_args(args...));
  }
};

}  // namespace reng
