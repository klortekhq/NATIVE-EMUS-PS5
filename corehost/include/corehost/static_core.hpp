#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <corehost/libretro_abi.hpp>

namespace corehost {

struct HostPaths {
  std::string system_dir;
  std::string save_dir;
};

struct InputState {
  std::uint16_t joypad_mask{};
  std::int16_t left_x{};
  std::int16_t left_y{};
  std::int16_t right_x{};
  std::int16_t right_y{};
  std::int16_t l2{};
  std::int16_t r2{};
  std::array<std::uint64_t, 8> keyboard{};
  std::int16_t mouse_x{};
  std::int16_t mouse_y{};
  std::int16_t mouse_wheel_x{};
  std::int16_t mouse_wheel_y{};
  std::uint16_t mouse_buttons{};

  [[nodiscard]] constexpr bool key_down(unsigned id) const noexcept {
    return id < 512u &&
           (keyboard[id >> 6u] & (std::uint64_t{1} << (id & 63u))) != 0;
  }
};

struct Hooks {
  std::function<void(const void*, unsigned, unsigned, std::size_t, lr::PixelFormat)> video;
  std::function<std::size_t(const std::int16_t*, std::size_t)> audio_batch;
  std::function<InputState(unsigned)> input;
  std::function<void(std::string_view)> log;
};

class StaticCore final {
public:
  StaticCore(std::string name, lr::StaticApi api, HostPaths paths, Hooks hooks);
  ~StaticCore();

  StaticCore(const StaticCore&) = delete;
  StaticCore& operator=(const StaticCore&) = delete;

  bool initialize(std::string& error);
  bool load_path(std::string_view path, std::string& error);
  void run_frame();
  void reset();
  void unload();
  void shutdown();

  bool save_state(std::vector<std::uint8_t>& out);
  bool load_state(const void* data, std::size_t size);

  [[nodiscard]] bool initialized() const noexcept { return initialized_; }
  [[nodiscard]] bool loaded() const noexcept { return loaded_; }
  [[nodiscard]] lr::PixelFormat pixel_format() const noexcept { return pixel_format_; }
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const lr::SystemInfo& system_info() const noexcept { return system_info_; }
  [[nodiscard]] const lr::SystemAvInfo& av_info() const noexcept { return av_info_; }

  void set_option(std::string key, std::string value);
  void* memory_data(unsigned id) noexcept;
  std::size_t memory_size(unsigned id) const noexcept;

private:
  friend bool environment_trampoline(unsigned, void*);
  friend void video_trampoline(const void*, unsigned, unsigned, std::size_t);
  friend void audio_sample_trampoline(std::int16_t, std::int16_t);
  friend std::size_t audio_batch_trampoline(const std::int16_t*, std::size_t);
  friend void input_poll_trampoline();
  friend std::int16_t input_state_trampoline(unsigned, unsigned, unsigned, unsigned);
  friend void log_trampoline(lr::LogLevel, const char*, ...);

  bool environment(unsigned cmd, void* data);
  void video(const void* data, unsigned width, unsigned height, std::size_t pitch);
  std::size_t audio(const std::int16_t* data, std::size_t frames);
  std::int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
  void register_variables(const lr::Variable* vars);
  void log(std::string_view message);

  std::string name_;
  lr::StaticApi api_{};
  HostPaths paths_{};
  Hooks hooks_{};
  bool initialized_{};
  bool loaded_{};
  bool options_dirty_{};
  lr::PixelFormat pixel_format_{lr::PixelFormat::xrgb1555};
  lr::SystemInfo system_info_{};
  lr::SystemAvInfo av_info_{};
  std::unordered_map<std::string, std::string> options_{};
  std::unordered_map<std::string, std::string> option_defaults_{};
  std::array<InputState, 4> input_cache_{};
  std::vector<std::uint8_t> content_buffer_{};
};

} // namespace corehost
