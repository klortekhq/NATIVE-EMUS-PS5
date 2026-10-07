#pragma once

#include "native_libretro_host.hpp"

namespace native_emu::libretro {

inline CoreApi linked_core_api() noexcept {
  CoreApi api{};
  api.set_environment = &retro_set_environment;
  api.set_video_refresh = &retro_set_video_refresh;
  api.set_audio_sample = &retro_set_audio_sample;
  api.set_audio_sample_batch = &retro_set_audio_sample_batch;
  api.set_input_poll = &retro_set_input_poll;
  api.set_input_state = &retro_set_input_state;
  api.init = &retro_init;
  api.deinit = &retro_deinit;
  api.get_system_info = &retro_get_system_info;
  api.get_system_av_info = &retro_get_system_av_info;
  api.set_controller_port_device = &retro_set_controller_port_device;
  api.load_game = &retro_load_game;
  api.unload_game = &retro_unload_game;
  api.run = &retro_run;
  api.reset = &retro_reset;
  api.get_memory_data = &retro_get_memory_data;
  api.get_memory_size = &retro_get_memory_size;
  return api;
}

struct PortConfig {
  const char* app_name{};
  const char* system_key{};
  const char* default_content{};
};

int run_linked_port(const PortConfig& config);

} // namespace native_emu::libretro
