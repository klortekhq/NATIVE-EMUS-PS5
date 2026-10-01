#include "../native/content.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace native_emus::ps1;

namespace mock {
std::array<std::uint8_t, 128> sram{};

void set_environment(corehost::lr::Environment) {}
void set_video(corehost::lr::VideoRefresh) {}
void set_audio(corehost::lr::AudioSample) {}
void set_audio_batch(corehost::lr::AudioBatch) {}
void set_input_poll(corehost::lr::InputPoll) {}
void set_input_state(corehost::lr::InputState) {}
void init() {}
void deinit() {}
void get_system_info(corehost::lr::SystemInfo* info) {
  info->library_name = "mock";
  info->library_version = "1";
  info->valid_extensions = "cue|chd|pbp";
  info->need_fullpath = true;
}
void set_port(unsigned, unsigned) {}
bool load_game(const corehost::lr::GameInfo*) { return true; }
void unload_game() {}
void get_av(corehost::lr::SystemAvInfo*) {}
void run() {}
void reset() {}
std::size_t serialize_size() { return 0; }
bool serialize(void*, std::size_t) { return false; }
bool unserialize(const void*, std::size_t) { return false; }
void* get_memory_data(unsigned id) {
  return id == corehost::lr::memory_save_ram ? sram.data() : nullptr;
}
std::size_t get_memory_size(unsigned id) {
  return id == corehost::lr::memory_save_ram ? sram.size() : 0;
}
}

int main() {
  assert(is_supported_ps1_content("game.CHD"));
  assert(is_supported_ps1_content("file:///tmp/game.cue"));
  assert(is_supported_ps1_content("disc.m3u"));
  assert(!is_supported_ps1_content("game.zip"));

  const auto root =
      std::filesystem::temp_directory_path() / "native-emus-ps1-content-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  const auto cue = root / "Ridge Test.cue";
  {
    std::ofstream f(cue);
    f << "FILE \"track.bin\" BINARY\n";
  }

  ContentLayout layout;
  layout.data_root = root / "data";
  layout.system_dir = layout.data_root / "system";
  layout.save_dir = layout.data_root / "saves";

  PreparedContent prepared;
  std::string error;
  assert(prepare_content(cue.string(), layout, prepared, error));
  assert(prepared.local_file);
  assert(prepared.core_path == cue.string());
  assert(prepared.save_ram_path.filename() == "Ridge Test.srm");

  PreparedContent rejected;
  assert(!prepare_content("smb://server/share/game.chd", layout, rejected, error));

  corehost::lr::StaticApi api{
      mock::set_environment,
      mock::set_video,
      mock::set_audio,
      mock::set_audio_batch,
      mock::set_input_poll,
      mock::set_input_state,
      mock::init,
      mock::deinit,
      mock::get_system_info,
      mock::set_port,
      mock::load_game,
      mock::unload_game,
      mock::get_av,
      mock::run,
      mock::reset,
      mock::serialize_size,
      mock::serialize,
      mock::unserialize,
      mock::get_memory_data,
      mock::get_memory_size,
  };
  corehost::StaticCore core("mock", api, {layout.system_dir.string(), layout.save_dir.string()}, {});
  assert(core.initialize(error));
  assert(core.load_path(cue.string(), error));

  for (std::size_t i = 0; i < mock::sram.size(); ++i)
    mock::sram[i] = static_cast<std::uint8_t>(i ^ 0x5a);

  SaveRamStore saves(prepared.save_ram_path);
  assert(saves.save(core, error));

  mock::sram.fill(0);
  assert(saves.load(core, error));
  for (std::size_t i = 0; i < mock::sram.size(); ++i)
    assert(mock::sram[i] == static_cast<std::uint8_t>(i ^ 0x5a));

  core.shutdown();
  std::filesystem::remove_all(root);
  return 0;
}
