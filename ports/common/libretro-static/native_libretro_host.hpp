#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <libretro.h>
#include <ps5rt/ps5rt.hpp>

namespace native_emu::libretro {

struct CoreApi {
  void (*set_environment)(retro_environment_t){};
  void (*set_video_refresh)(retro_video_refresh_t){};
  void (*set_audio_sample)(retro_audio_sample_t){};
  void (*set_audio_sample_batch)(retro_audio_sample_batch_t){};
  void (*set_input_poll)(retro_input_poll_t){};
  void (*set_input_state)(retro_input_state_t){};
  void (*init)(){};
  void (*deinit)(){};
  void (*get_system_info)(retro_system_info*){};
  void (*get_system_av_info)(retro_system_av_info*){};
  void (*set_controller_port_device)(unsigned, unsigned){};
  bool (*load_game)(const retro_game_info*){};
  void (*unload_game)(){};
  void (*run)(){};
  void (*reset)(){};
  void* (*get_memory_data)(unsigned){};
  std::size_t (*get_memory_size)(unsigned){};
};

struct Paths {
  std::string content;
  std::string system_dir;
  std::string save_dir;
  std::string cache_dir;
};

class NativeHost {
public:
  NativeHost(CoreApi api, Paths paths);
  NativeHost(const NativeHost&) = delete;
  NativeHost& operator=(const NativeHost&) = delete;
  ~NativeHost();

  ps5rt::Result start() noexcept;
  bool run_frame() noexcept;
  void stop() noexcept;

  [[nodiscard]] const std::string& error() const noexcept { return error_; }
  [[nodiscard]] bool running() const noexcept { return running_; }

private:
  static bool environment_cb(unsigned cmd, void* data);
  static void video_cb(const void* data, unsigned width, unsigned height, std::size_t pitch);
  static void audio_sample_cb(std::int16_t left, std::int16_t right);
  static std::size_t audio_batch_cb(const std::int16_t* data, std::size_t frames);
  static void input_poll_cb();
  static std::int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id);
  static void log_cb(enum retro_log_level level, const char* fmt, ...);

  bool environment(unsigned cmd, void* data) noexcept;
  void video(const void* data, unsigned width, unsigned height, std::size_t pitch) noexcept;
  std::size_t audio(const std::int16_t* data, std::size_t frames) noexcept;
  void poll_input() noexcept;
  std::int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) const noexcept;

  bool load_content(retro_game_info& info) noexcept;
  void load_sram() noexcept;
  void save_sram() noexcept;
  std::string save_path() const;

  CoreApi api_{};
  Paths paths_{};
  retro_system_info system_info_{};
  retro_system_av_info av_info_{};
  retro_pixel_format pixel_format_{RETRO_PIXEL_FORMAT_0RGB1555};

  ps5rt::VideoDevice video_{};
  ps5rt::AudioDevice audio_{};
  ps5rt::InputSnapshot input_{};

  std::unordered_map<std::string, std::string> variables_{};
  std::vector<std::uint8_t> content_buffer_{};

  std::string error_{};
  bool initialized_{};
  bool loaded_{};
  bool input_ready_{};
  bool running_{};
  bool quit_{};
  std::uint64_t frames_{};
};

} // namespace native_emu::libretro
