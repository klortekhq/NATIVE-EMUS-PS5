#pragma once

#include <cstdarg>
#include <cstdint>
#include <string_view>

namespace ps5rt {

enum class LogLevel : std::uint8_t {
  trace,
  debug,
  info,
  warning,
  error,
  fatal,
};

using LogSink = void(*)(LogLevel level, std::string_view component, std::string_view message);

void set_log_sink(LogSink sink) noexcept;
void log(LogLevel level, std::string_view component, std::string_view message) noexcept;

} // namespace ps5rt
