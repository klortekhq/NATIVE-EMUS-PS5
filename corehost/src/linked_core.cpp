#include <corehost/linked_core.hpp>

extern "C" {
void retro_set_environment(corehost::lr::Environment);
void retro_set_video_refresh(corehost::lr::VideoRefresh);
void retro_set_audio_sample(corehost::lr::AudioSample);
void retro_set_audio_sample_batch(corehost::lr::AudioBatch);
void retro_set_input_poll(corehost::lr::InputPoll);
void retro_set_input_state(corehost::lr::InputState);
void retro_init(void);
void retro_deinit(void);
void retro_get_system_info(corehost::lr::SystemInfo*);
void retro_set_controller_port_device(unsigned, unsigned);
bool retro_load_game(const corehost::lr::GameInfo*);
void retro_unload_game(void);
void retro_get_system_av_info(corehost::lr::SystemAvInfo*);
void retro_run(void);
void retro_reset(void);
std::size_t retro_serialize_size(void);
bool retro_serialize(void*, std::size_t);
bool retro_unserialize(const void*, std::size_t);
void* retro_get_memory_data(unsigned);
std::size_t retro_get_memory_size(unsigned);
}

namespace corehost {

lr::StaticApi linked_core_api() noexcept {
  return {
      retro_set_environment,
      retro_set_video_refresh,
      retro_set_audio_sample,
      retro_set_audio_sample_batch,
      retro_set_input_poll,
      retro_set_input_state,
      retro_init,
      retro_deinit,
      retro_get_system_info,
      retro_set_controller_port_device,
      retro_load_game,
      retro_unload_game,
      retro_get_system_av_info,
      retro_run,
      retro_reset,
      retro_serialize_size,
      retro_serialize,
      retro_unserialize,
      retro_get_memory_data,
      retro_get_memory_size,
  };
}

} // namespace corehost
