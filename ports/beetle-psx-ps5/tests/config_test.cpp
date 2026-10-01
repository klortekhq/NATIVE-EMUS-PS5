#include "../native/config.hpp"

#include <corehost/static_core.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace mock {
corehost::lr::Environment env{};

void set_environment(corehost::lr::Environment cb) { env = cb; }
void set_video(corehost::lr::VideoRefresh) {}
void set_audio(corehost::lr::AudioSample) {}
void set_audio_batch(corehost::lr::AudioBatch) {}
void set_input_poll(corehost::lr::InputPoll) {}
void set_input_state(corehost::lr::InputState) {}
void init() {}
void deinit() {}
void get_system_info(corehost::lr::SystemInfo* info) {
  info->library_name = "config-test";
  info->library_version = "1";
  info->valid_extensions = "cue";
  info->need_fullpath = true;
}
bool load_game(const corehost::lr::GameInfo*) { return true; }
void unload_game() {}
void get_av(corehost::lr::SystemAvInfo* info) {
  info->timing.sample_rate = 44100.0;
}
void run() {}

std::string option(const char* key) {
  corehost::lr::Variable v{key, nullptr};
  assert(env && env(corehost::lr::env_get_variable, &v));
  assert(v.value);
  return v.value;
}
} // namespace mock

int main() {
  using native_emus::ps1::PortConfig;

  const auto root =
      std::filesystem::temp_directory_path() / "native-emus-ps1-config-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);

  PortConfig config;
  std::string error;

  // Missing file: valid defaults.
  assert(native_emus::ps1::load_port_config(
      root / "missing.ini", config, error));
  assert(config.region == "auto");
  assert(config.bios_override == "disabled");
  assert(config.skip_bios == "disabled");
  assert(config.internal_resolution == "1x(native)");

  const auto path = root / "ps1.ini";
  {
    std::ofstream out(path);
    out << "# standalone PS1 preferences\n"
        << "region = pal\n"
        << "bios = openbios\n"
        << "skip_bios = false\n"
        << "internal_resolution = 4x\n"
        << "future_option = ignored-for-forward-compat\n";
  }

  assert(native_emus::ps1::load_port_config(path, config, error));
  assert(config.region == "pal");
  assert(config.bios_override == "openbios");
  assert(config.skip_bios == "disabled");
  assert(config.internal_resolution == "4x");

  corehost::lr::StaticApi api{};
  api.set_environment = mock::set_environment;
  api.set_video_refresh = mock::set_video;
  api.set_audio_sample = mock::set_audio;
  api.set_audio_sample_batch = mock::set_audio_batch;
  api.set_input_poll = mock::set_input_poll;
  api.set_input_state = mock::set_input_state;
  api.init = mock::init;
  api.deinit = mock::deinit;
  api.get_system_info = mock::get_system_info;
  api.load_game = mock::load_game;
  api.unload_game = mock::unload_game;
  api.get_system_av_info = mock::get_av;
  api.run = mock::run;

  corehost::StaticCore core(
      "config-test", api, {"/system", "/save"}, {});
  native_emus::ps1::apply_port_config(core, config);
  assert(core.initialize(error));

  // User preferences arrive at the core.
  assert(mock::option("beetle_psx_hw_region") == "pal");
  assert(mock::option("beetle_psx_hw_override_bios") == "openbios");
  assert(mock::option("beetle_psx_hw_skip_bios") == "disabled");
  assert(mock::option("beetle_psx_hw_internal_resolution") == "4x");

  // Architecture cannot be downgraded through the standalone INI.
  assert(mock::option("beetle_psx_hw_renderer") == "hardware_vk");
  assert(mock::option("beetle_psx_hw_cpu_dynarec") == "execute");

  core.shutdown();

  // Invalid values must fail rather than silently changing emulation behavior.
  {
    std::ofstream out(path, std::ios::trunc);
    out << "region = moon\n";
  }
  assert(!native_emus::ps1::load_port_config(path, config, error));
  assert(error.find("invalid PS1 region") != std::string::npos);

  std::filesystem::remove_all(root);
  return 0;
}
