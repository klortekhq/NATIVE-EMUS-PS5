#include <ps5rt/log.hpp>

#include <cassert>
#include <string>

namespace {
int calls = 0;
ps5rt::LogLevel seen_level = ps5rt::LogLevel::trace;
std::string seen_component;
std::string seen_message;

void sink(
    ps5rt::LogLevel level,
    std::string_view component,
    std::string_view message) {
  ++calls;
  seen_level = level;
  seen_component.assign(component);
  seen_message.assign(message);
}
}

int main() {
  ps5rt::set_log_sink(sink);
  ps5rt::log(ps5rt::LogLevel::warning, "ps1", "test-message");
  assert(calls == 1);
  assert(seen_level == ps5rt::LogLevel::warning);
  assert(seen_component == "ps1");
  assert(seen_message == "test-message");
  ps5rt::set_log_sink(nullptr);
  return 0;
}
