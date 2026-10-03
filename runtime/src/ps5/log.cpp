#include <ps5rt/log.hpp>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <mutex>
#include <string>

namespace ps5rt {
namespace {

std::atomic<LogSink> g_sink{nullptr};
std::mutex g_file_mutex;
std::FILE* g_file{};

const char* level_name(LogLevel level) noexcept {
  switch (level) {
    case LogLevel::trace:   return "TRACE";
    case LogLevel::debug:   return "DEBUG";
    case LogLevel::info:    return "INFO";
    case LogLevel::warning: return "WARN";
    case LogLevel::error:   return "ERROR";
    case LogLevel::fatal:   return "FATAL";
  }
  return "LOG";
}

int printable_size(std::size_t size) noexcept {
  return static_cast<int>(
      std::min<std::size_t>(
          size,
          static_cast<std::size_t>(INT_MAX)));
}

void write_line(
    std::FILE* stream,
    LogLevel level,
    std::string_view component,
    std::string_view message) noexcept {
  if (stream == nullptr)
    return;

  std::fprintf(
      stream,
      "[%s] %.*s: %.*s\n",
      level_name(level),
      printable_size(component.size()),
      component.data(),
      printable_size(message.size()),
      message.data());
  std::fflush(stream);
}

} // namespace

void set_log_sink(LogSink sink) noexcept {
  g_sink.store(sink, std::memory_order_release);
}

Result initialize_file_logging(
    std::string_view path) noexcept {
  if (path.empty()) {
    return {
        ErrorCode::invalid_argument,
        0,
        "log file path is empty"};
  }
  if (path.find('\0') != std::string_view::npos) {
    return {
        ErrorCode::invalid_argument,
        0,
        "log file path contains NUL"};
  }

  const std::string owned_path(path);
  std::FILE* candidate =
      std::fopen(owned_path.c_str(), "ab");
  if (candidate == nullptr) {
    return {
        ErrorCode::io_error,
        errno,
        "log file open failed"};
  }

  std::scoped_lock lock(g_file_mutex);
  if (g_file != nullptr)
    std::fclose(g_file);
  g_file = candidate;
  return Result::success();
}

void shutdown_file_logging() noexcept {
  std::scoped_lock lock(g_file_mutex);
  if (g_file != nullptr) {
    std::fflush(g_file);
    std::fclose(g_file);
    g_file = nullptr;
  }
}

void log(
    LogLevel level,
    std::string_view component,
    std::string_view message) noexcept {
  const auto sink =
      g_sink.load(std::memory_order_acquire);
  if (sink != nullptr) {
    sink(level, component, message);
  } else {
    // stderr stays the dependency-free launcher/kernel-log fallback.
    write_line(stderr, level, component, message);
  }

  // Persistent logging is deliberately independent of a custom sink so an
  // app can retain both telemetry/UI logging and a local diagnostic trail.
  std::scoped_lock lock(g_file_mutex);
  write_line(
      g_file,
      level,
      component,
      message);
}

} // namespace ps5rt
