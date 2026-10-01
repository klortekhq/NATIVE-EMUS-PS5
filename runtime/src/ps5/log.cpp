#include <ps5rt/log.hpp>

#include <atomic>
#include <cstdio>

namespace ps5rt {
namespace {

std::atomic<LogSink> g_sink{nullptr};

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

} // namespace

void set_log_sink(LogSink sink) noexcept {
  g_sink.store(sink, std::memory_order_release);
}

void log(
    LogLevel level,
    std::string_view component,
    std::string_view message) noexcept {
  if (const auto sink = g_sink.load(std::memory_order_acquire)) {
    sink(level, component, message);
    return;
  }

  // stderr is intentionally the dependency-free fallback. Jailbreak/homebrew
  // launchers commonly capture it; a richer klog/network sink can be installed
  // by the app without coupling emulator cores to that transport.
  std::fprintf(
      stderr, "[%s] %.*s: %.*s\n",
      level_name(level),
      static_cast<int>(component.size()), component.data(),
      static_cast<int>(message.size()), message.data());
  std::fflush(stderr);
}

} // namespace ps5rt
