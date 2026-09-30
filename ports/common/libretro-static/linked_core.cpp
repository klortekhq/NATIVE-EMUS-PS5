#include "linked_core.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

namespace native_emu::libretro {
namespace {

bool exists(const std::string& path) {
  if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
    std::fclose(f);
    return true;
  }
  return false;
}

void ensure_dir(const std::string& path) {
  if (::mkdir(path.c_str(), 0777) != 0 && errno != EEXIST)
    std::fprintf(stderr, "mkdir failed: %s errno=%d\n", path.c_str(), errno);
}

std::string read_path_file(const std::string& path) {
  char value[1024]{};
  if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
    if (std::fgets(value, sizeof(value), f))
      value[std::strcspn(value, "\r\n")] = '\0';
    std::fclose(f);
  }
  return value;
}

std::string pick_content(
    const std::string& root,
    const std::string& usb_relative,
    const std::string& filename) {
  for (const auto& cfg : {
           root + "/rom-path.txt",
           std::string("/app0/rom-path.txt")}) {
    const auto chosen = read_path_file(cfg);
    if (!chosen.empty() && exists(chosen))
      return chosen;
  }

  for (const auto& candidate : {
           root + "/" + filename,
           root + "/test-" + filename,
           std::string("/mnt/usb0/") + usb_relative + "/" + filename,
           std::string("/mnt/usb1/") + usb_relative + "/" + filename,
           std::string("/app0/") + filename}) {
    if (exists(candidate))
      return candidate;
  }

  return root + "/" + filename;
}

} // namespace

int run_linked_port(const PortConfig& config) {
  if (!config.app_name || !config.system_key || !config.default_content)
    return 2;

  const std::string base = "/data/NATIVE-EMUS-PS5";
  const std::string root = base + "/" + config.system_key;
  const std::string saves = root + "/saves";
  const std::string cache = root + "/cache";
  const std::string system = root + "/system";
  const std::string logs = root + "/logs";

  ensure_dir(base);
  ensure_dir(root);
  ensure_dir(saves);
  ensure_dir(cache);
  ensure_dir(system);
  ensure_dir(logs);

  const std::string logfile = logs + "/native.log";
  (void)std::freopen(logfile.c_str(), "a", stderr);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  std::fprintf(stderr, "%s: process entered\n", config.app_name);

  ps5rt::AppInfo app{};
  const ps5rt::AppConfig app_config{
      config.app_name,
      base,
      true,
      false,
      true};
  const auto app_result = ps5rt::initialize_app(app_config, app);
  if (!app_result) {
    std::fprintf(stderr, "app init failed: %.*s (%d)\n",
                 static_cast<int>(app_result.message.size()), app_result.message.data(),
                 app_result.native_code);
    return 10;
  }

  const std::string relative =
      std::string("NATIVE-EMUS-PS5/") + config.system_key;
  Paths paths{};
  paths.content = pick_content(root, relative, config.default_content);
  paths.system_dir = system;
  paths.save_dir = saves;
  paths.cache_dir = cache;

  std::fprintf(stderr, "content=%s\n", paths.content.c_str());

  NativeHost host(linked_core_api(), std::move(paths));
  const auto started = host.start();
  if (!started) {
    std::fprintf(stderr, "core start failed: %s\n", host.error().c_str());
    ps5rt::shutdown_app();
    return 11;
  }

  while (host.run_frame()) {
  }

  if (!host.error().empty())
    std::fprintf(stderr, "core stopped with error: %s\n", host.error().c_str());
  else
    std::fprintf(stderr, "core clean exit\n");

  host.stop();
  ps5rt::shutdown_app();
  return host.error().empty() ? 0 : 12;
}

} // namespace native_emu::libretro
