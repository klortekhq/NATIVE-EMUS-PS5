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
std::array<std::uint8_t, 64> state{};

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
std::size_t serialize_size() { return state.size(); }
bool serialize(void* data, std::size_t size) {
  if (!data || size != state.size())
    return false;
  std::memcpy(data, state.data(), state.size());
  return true;
}
bool unserialize(const void* data, std::size_t size) {
  if (!data || size != state.size())
    return false;
  std::memcpy(state.data(), data, state.size());
  return true;
}
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

  const std::string remote_id(64, 'a');
  const std::string remote_chd =
      "emus://192.168.1.50:8787/" + remote_id + "/Metal Gear Solid.chd";
  assert(is_supported_ps1_content(remote_chd));
  assert(!is_supported_ps1_content(
      "emus://192.168.1.50:8787/" + remote_id));

  const auto root =
      std::filesystem::temp_directory_path() / "native-emus-ps1-content-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  const auto cue = root / "Ridge Test.cue";
  const auto disc1 = root / "disc1.chd";
  const auto disc2 = root / "disc2.cue";
  {
    std::ofstream f(cue);
    f << "FILE \"track.bin\" BINARY\n";
  }
  {
    std::ofstream f(disc1, std::ios::binary);
    f << "CHD";
  }
  {
    std::ofstream f(disc2);
    f << "FILE \"disc2.bin\" BINARY\n";
  }

  ContentLayout layout;
  layout.data_root = root / "data";
  layout.system_dir = layout.data_root / "system";
  layout.save_dir = layout.data_root / "saves";
  layout.state_dir = layout.data_root / "states";

  PreparedContent prepared;
  std::string error;
  assert(prepare_content(cue.string(), layout, prepared, error));
  assert(prepared.local_file);
  assert(prepared.core_path == cue.string());
  assert(prepared.save_ram_path.filename() == "Ridge Test.srm");
  assert(prepared.state_path.filename() == "Ridge Test.state0");

  const auto playlist = root / "Final Fantasy Test.m3u";
  {
    std::ofstream f(playlist);
    f << "# multi-disc regression\n";
    f << "disc1.chd\n";
    f << "disc2.cue\n";
  }
  PreparedContent multi;
  assert(prepare_content(playlist.string(), layout, multi, error));
  assert(multi.playlist_entries.size() == 2);
  assert(multi.playlist_entries[0] == disc1);
  assert(multi.playlist_entries[1] == disc2);
  assert(multi.save_ram_path.filename() == "Final Fantasy Test.srm");
  assert(multi.state_path.filename() == "Final Fantasy Test.state0");

  const auto bad_playlist = root / "bad.m3u";
  {
    std::ofstream f(bad_playlist);
    f << "smb://server/share/disc1.chd\n";
  }
  PreparedContent bad_multi;
  assert(!prepare_content(bad_playlist.string(), layout, bad_multi, error));
  assert(error == "PS1 M3U network entries require the ps5rt VFS bridge");

  const auto nested_playlist = root / "nested.m3u";
  {
    std::ofstream f(nested_playlist);
    f << "Final Fantasy Test.m3u\n";
  }
  assert(!prepare_content(nested_playlist.string(), layout, bad_multi, error));
  assert(error == "nested PS1 M3U playlists are not supported");

  PreparedContent remote;
  assert(prepare_content(remote_chd, layout, remote, error));
  assert(!remote.local_file);
  assert(remote.core_path == remote_chd);
  assert(remote.save_ram_path.filename() == "Metal Gear Solid.srm");
  assert(remote.state_path.filename() == "Metal Gear Solid.state0");

  const std::string remote_m3u =
      "emus://192.168.1.50:8787/" + remote_id + "/Final Fantasy.m3u";
  PreparedContent remote_playlist;
  assert(!prepare_content(remote_m3u, layout, remote_playlist, error));
  assert(error == "remote PS1 M3U playlists are not supported yet");

  PreparedContent rejected;
  assert(!prepare_content("smb://server/share/game.chd", layout, rejected, error));
  assert(error == "unsupported PS1 URI scheme");

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

  // Replacing an existing memory-card image must remain atomic and must not
  // leave the temporary file behind.
  for (std::size_t i = 0; i < mock::sram.size(); ++i)
    mock::sram[i] = static_cast<std::uint8_t>(0xa5 ^ i);
  assert(saves.save(core, error));
  assert(!std::filesystem::exists(prepared.save_ram_path.string() + ".tmp"));

  mock::sram.fill(0);
  assert(saves.load(core, error));
  for (std::size_t i = 0; i < mock::sram.size(); ++i)
    assert(mock::sram[i] == static_cast<std::uint8_t>(0xa5 ^ i));

  // A truncated/corrupt card must be rejected before touching live core RAM.
  {
    std::ofstream corrupt(prepared.save_ram_path, std::ios::binary | std::ios::trunc);
    const std::array<char, 7> short_card{'P','S','1','B','A','D','!'};
    corrupt.write(short_card.data(), static_cast<std::streamsize>(short_card.size()));
  }
  mock::sram.fill(0x3c);
  assert(!saves.load(core, error));
  assert(error == "save RAM size does not match core");
  for (const auto byte : mock::sram)
    assert(byte == 0x3c);

  // Slot-0 save states use the core's real serialize/unserialize callbacks,
  // but the host wraps them in a versioned header and atomic file replacement.
  for (std::size_t i = 0; i < mock::state.size(); ++i)
    mock::state[i] = static_cast<std::uint8_t>((i * 3u) ^ 0x7c);

  SaveStateStore states(prepared.state_path);
  assert(states.save(core, error));
  assert(std::filesystem::exists(prepared.state_path));
  assert(!std::filesystem::exists(prepared.state_path.string() + ".tmp"));

  const auto expected_state = mock::state;
  mock::state.fill(0);
  assert(states.load(core, error));
  assert(mock::state == expected_state);

  // Corrupt/truncated state files must be rejected before unserialize can
  // overwrite the running emulator state.
  {
    std::ofstream corrupt(
        prepared.state_path, std::ios::binary | std::ios::trunc);
    corrupt << "NEPS1ST";
  }
  mock::state.fill(0x55);
  assert(!states.load(core, error));
  assert(error == "invalid PS1 save-state header");
  for (const auto byte : mock::state)
    assert(byte == 0x55);

  core.shutdown();
  std::filesystem::remove_all(root);
  return 0;
}
