#include "reng/logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace reng {

namespace {
std::mutex gMutex;
std::ofstream gFile;
bool gInitialized = false;

std::string levelToString(RengLogger::Level level) {
  switch (level) {
    case RengLogger::Level::Verbose:
      return "Verbose";
    case RengLogger::Level::Info:
      return "Info";
    case RengLogger::Level::Warning:
      return "Warning";
    case RengLogger::Level::Error:
      return "Error";
    default:
      return "Info";
  }
}

std::string timestampNow() {
  using Clock = std::chrono::system_clock;
  auto now = Clock::now();
  std::time_t time = Clock::to_time_t(now);
  std::tm local{};
#if defined(_WIN32)
  localtime_s(&local, &time);
#else
  localtime_r(&time, &local);
#endif
  std::ostringstream oss;
  oss << std::put_time(&local, "%Y-%m-%d-%H-%M-%S");
  return oss.str();
}

void writeLine(const std::string& line) {
  std::cout << line << '\n';
  if (gFile.is_open()) {
    gFile << line << '\n';
    gFile.flush();
  }
}
}  // namespace

bool RengLogger::init(const std::string& filePath) {
  std::scoped_lock lock(gMutex);
  if (gInitialized) {
    return true;
  }
  std::filesystem::path path(filePath);
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  gFile.open(filePath, std::ios::out | std::ios::app);
  gInitialized = gFile.is_open();
  return gInitialized;
}

void RengLogger::shutdown() {
  std::scoped_lock lock(gMutex);
  if (gFile.is_open()) {
    gFile.flush();
    gFile.close();
  }
  gInitialized = false;
}

void RengLogger::log(Level level, std::string_view message) {
  std::scoped_lock lock(gMutex);
  const std::string line =
      "[" + timestampNow() + "] [" + levelToString(level) + "] " +
      std::string(message);
  writeLine(line);
}

void RengLogger::logWithLocation(Level level, std::string_view message,
                                 std::string_view file, int line) {
  std::string annotated(message);
  annotated += " (";
  annotated.append(file.data(), file.size());
  annotated += ":";
  annotated += std::to_string(line);
  annotated += ")";
  log(level, annotated);
}

std::string RengLogger::formatMessage(std::string_view format) {
  return std::string(format);
}

}  // namespace reng
