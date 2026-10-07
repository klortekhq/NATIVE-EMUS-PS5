#include <ps5rt/tls.hpp>

#include <cassert>
#include <iostream>

int main() {
  ps5rt::TlsInfo info{};

  auto before =
      ps5rt::register_current_thread_tls();
  assert(!before);
  assert(
      before.code ==
      ps5rt::ErrorCode::invalid_argument);

  const auto automatic =
      ps5rt::initialize_tls(
          ps5rt::TlsMode::automatic,
          info);
  assert(automatic);
  assert(
      info.mode ==
      ps5rt::TlsMode::compiler_rt_emutls);
  assert(!info.thread_registration_required);

  const auto registered =
      ps5rt::register_current_thread_tls();
  assert(registered);
  ps5rt::unregister_current_thread_tls();

  ps5rt::TlsInfo repeated_info{};
  const auto repeated =
      ps5rt::initialize_tls(
          ps5rt::TlsMode::compiler_rt_emutls,
          repeated_info);
  assert(repeated);
  assert(
      repeated_info.mode ==
      ps5rt::TlsMode::compiler_rt_emutls);

  ps5rt::TlsInfo unsupported_info{};
  const auto native =
      ps5rt::initialize_tls(
          ps5rt::TlsMode::native,
          unsupported_info);
  assert(!native);
  assert(
      native.code ==
      ps5rt::ErrorCode::unsupported);

  const auto compatibility =
      ps5rt::initialize_tls(
          ps5rt::TlsMode::compatibility_shim,
          unsupported_info);
  assert(!compatibility);
  assert(
      compatibility.code ==
      ps5rt::ErrorCode::unsupported);

  ps5rt::shutdown_tls();
  const auto after =
      ps5rt::register_current_thread_tls();
  assert(!after);
  assert(
      after.code ==
      ps5rt::ErrorCode::invalid_argument);

  std::cout
      << "ps5rt TLS policy tests passed\n";
  return 0;
}
