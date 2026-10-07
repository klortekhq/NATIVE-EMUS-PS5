#pragma once

#include <cstdarg>
#include <cstdint>
#include <string_view>

#include <ps5rt/result.hpp>

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

// Optional persistent sink. The parent directory must already exist; ps5rt
// deliberately does not guess a title-specific storage root here.
Result initialize_file_logging(std::string_view path) noexcept;
void shutdown_file_logging() noexcept;

void log(LogLevel level, std::string_view component, std::string_view message) noexcept;

} // namespace ps5rt
