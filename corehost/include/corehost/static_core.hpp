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

  void set_option(std::string key, std::string value);

private:
  friend bool environment_trampoline(unsigned, void*);
  friend void video_trampoline(const void*, unsigned, unsigned, std::size_t);
  friend void audio_sample_trampoline(std::int16_t, std::int16_t);
  friend std::size_t audio_batch_trampoline(const std::int16_t*, std::size_t);
  friend void input_poll_trampoline();
  friend std::int16_t input_state_trampoline(unsigned, unsigned, unsigned, unsigned);

  bool environment(unsigned cmd, void* data);
  void video(const void* data, unsigned width, unsigned height, std::size_t pitch);
  std::size_t audio(const std::int16_t* data, std::size_t frames);
  std::int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
  void register_variables(const lr::Variable* vars);

  std::string name_;
  lr::StaticApi api_{};
  HostPaths paths_{};
  Hooks hooks_{};
  bool initialized_{};
  bool loaded_{};
  lr::PixelFormat pixel_format_{lr::PixelFormat::xrgb1555};
  std::unordered_map<std::string, std::string> options_{};
  std::unordered_map<std::string, std::string> option_defaults_{};
  std::array<InputState, 4> input_cache_{};
};

} // namespace corehost
