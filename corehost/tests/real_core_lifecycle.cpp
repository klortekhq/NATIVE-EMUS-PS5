#include <corehost/static_core.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

extern "C" {
unsigned retro_api_version(void);
void retro_set_environment(corehost::lr::Environment);
void retro_set_video_refresh(corehost::lr::VideoRefresh);
void retro_set_audio_sample(corehost::lr::AudioSample);
void retro_set_audio_sample_batch(corehost::lr::AudioBatch);
void retro_set_input_poll(corehost::lr::InputPoll);
void retro_set_input_state(corehost::lr::InputState);
void retro_init(void);
void retro_deinit(void);
bool retro_load_game(const corehost::lr::GameInfo*);
void retro_unload_game(void);
void retro_run(void);
void retro_reset(void);
std::size_t retro_serialize_size(void);
bool retro_serialize(void*, std::size_t);
bool retro_unserialize(const void*, std::size_t);
}

int main(int argc, char** argv) {
  const std::string name = argc > 1 ? argv[1] : "real-core";

  if (retro_api_version() != corehost::lr::api_version) {
    std::cerr << name << ": unexpected libretro ABI version\n";
    return 2;
  }

  corehost::lr::StaticApi api{
      retro_set_environment,
      retro_set_video_refresh,
      retro_set_audio_sample,
      retro_set_audio_sample_batch,
      retro_set_input_poll,
      retro_set_input_state,
      retro_init,
      retro_deinit,
      retro_load_game,
      retro_unload_game,
      retro_run,
      retro_reset,
      retro_serialize_size,
      retro_serialize,
      retro_unserialize,
  };

  corehost::Hooks hooks{};
  hooks.video = [](const void*, unsigned, unsigned, std::size_t, corehost::lr::PixelFormat) {};
  hooks.audio_batch = [](const std::int16_t*, std::size_t frames) { return frames; };
  hooks.input = [](unsigned) { return corehost::InputState{}; };

  corehost::StaticCore core(
      name, api,
      {"/tmp/native-emus-system", "/tmp/native-emus-save"},
      std::move(hooks));

  std::string error;
  if (!core.initialize(error)) {
    std::cerr << name << ": init failed: " << error << "\n";
    return 3;
  }

  if (!core.initialized()) {
    std::cerr << name << ": lifecycle invariant failed\n";
    return 4;
  }

  core.shutdown();
  std::cout << name << ": static core lifecycle PASS\n";
  return 0;
}
