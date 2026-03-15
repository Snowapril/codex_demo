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
  static void logVerboseImpl(std::string_view file, int line,
                             std::string_view format, Args&&... args) {
    logWithLocation(Level::Verbose,
                    formatMessage(format, std::forward<Args>(args)...),
                    file, line);
  }

  template <typename... Args>
  static void logInfoImpl(std::string_view file, int line,
                          std::string_view format, Args&&... args) {
    logWithLocation(Level::Info,
                    formatMessage(format, std::forward<Args>(args)...),
                    file, line);
  }

  template <typename... Args>
  static void logWarningImpl(std::string_view file, int line,
                             std::string_view format, Args&&... args) {
    logWithLocation(Level::Warning,
                    formatMessage(format, std::forward<Args>(args)...),
                    file, line);
  }

  template <typename... Args>
  static void logErrorImpl(std::string_view file, int line,
                           std::string_view format, Args&&... args) {
    logWithLocation(Level::Error,
                    formatMessage(format, std::forward<Args>(args)...),
                    file, line);
  }

 private:
  static std::string formatMessage(std::string_view format);
  static void logWithLocation(Level level, std::string_view message,
                              std::string_view file, int line);

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

#define logVerbose(...) logVerboseImpl(__FILE__, __LINE__, __VA_ARGS__)
#define logInfo(...) logInfoImpl(__FILE__, __LINE__, __VA_ARGS__)
#define logWarning(...) logWarningImpl(__FILE__, __LINE__, __VA_ARGS__)
#define logError(...) logErrorImpl(__FILE__, __LINE__, __VA_ARGS__)
