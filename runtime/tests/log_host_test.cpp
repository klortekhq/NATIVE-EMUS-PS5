#include <ps5rt/log.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
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

  const auto root =
      std::filesystem::temp_directory_path() /
      "ps5rt-log-host-test";
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  std::filesystem::create_directories(root);
  const auto log_path = root / "runtime.log";

  assert(ps5rt::initialize_file_logging(
      log_path.string()));
  ps5rt::log(
      ps5rt::LogLevel::error,
      "runtime",
      "persistent-message");
  ps5rt::shutdown_file_logging();

  std::ifstream input(log_path, std::ios::binary);
  const std::string persisted(
      (std::istreambuf_iterator<char>(input)),
      std::istreambuf_iterator<char>());
  assert(
      persisted.find(
          "[ERROR] runtime: persistent-message\n") !=
      std::string::npos);

  // Reopen uses append semantics rather than destroying the prior evidence.
  assert(ps5rt::initialize_file_logging(
      log_path.string()));
  ps5rt::log(
      ps5rt::LogLevel::info,
      "runtime",
      "second-message");
  ps5rt::shutdown_file_logging();

  std::ifstream input2(log_path, std::ios::binary);
  const std::string appended(
      (std::istreambuf_iterator<char>(input2)),
      std::istreambuf_iterator<char>());
  assert(
      appended.find("persistent-message") !=
      std::string::npos);
  assert(
      appended.find("second-message") !=
      std::string::npos);

  assert(!ps5rt::initialize_file_logging(""));
  ps5rt::set_log_sink(nullptr);
  std::filesystem::remove_all(root, ec);
  return 0;
}
