#pragma once
#include <cstddef>
#include <cstdint>

namespace corehost::lr {

constexpr unsigned api_version = 1;
constexpr unsigned environment_experimental = 0x10000;

constexpr unsigned env_get_system_directory = 9;
constexpr unsigned env_set_pixel_format = 10;
constexpr unsigned env_get_variable = 15;
constexpr unsigned env_set_variables = 16;
constexpr unsigned env_get_variable_update = 17;
constexpr unsigned env_get_log_interface = 27;
constexpr unsigned env_get_save_directory = 31;
constexpr unsigned env_get_vfs_interface = 45 | environment_experimental;
constexpr unsigned env_get_input_bitmasks = 51 | environment_experimental;
constexpr unsigned env_get_core_options_version = 52;

constexpr unsigned device_none = 0;
constexpr unsigned device_joypad = 1;
constexpr unsigned device_mouse = 2;
constexpr unsigned device_keyboard = 3;
constexpr unsigned device_analog = 5;
constexpr unsigned device_joypad_mask = 256;
constexpr unsigned analog_left = 0;
constexpr unsigned analog_right = 1;
constexpr unsigned analog_button = 2;
constexpr unsigned analog_x = 0;
constexpr unsigned analog_y = 1;

constexpr unsigned mouse_x = 0;
constexpr unsigned mouse_y = 1;
constexpr unsigned mouse_left = 2;
constexpr unsigned mouse_right = 3;
constexpr unsigned mouse_wheel_up = 4;
constexpr unsigned mouse_wheel_down = 5;
constexpr unsigned mouse_middle = 6;
constexpr unsigned mouse_wheel_left = 7;
constexpr unsigned mouse_wheel_right = 8;
constexpr unsigned mouse_button_4 = 9;
constexpr unsigned mouse_button_5 = 10;

enum class PixelFormat : unsigned {
  xrgb1555 = 0,
  xrgb8888 = 1,
  rgb565 = 2,
};

enum class LogLevel : int {
  debug = 0,
  info = 1,
  warn = 2,
  error = 3,
};

using LogPrintf = void(*)(LogLevel, const char*, ...);

struct LogCallback {
  LogPrintf log{};
};

struct Variable {
  const char* key;
  const char* value;
};

struct GameInfo {
  const char* path;
  const void* data;
  std::size_t size;
  const char* meta;
};

struct GameGeometry {
  unsigned base_width{};
  unsigned base_height{};
  unsigned max_width{};
  unsigned max_height{};
  float aspect_ratio{};
};

struct SystemTiming {
  double fps{};
  double sample_rate{};
};

struct SystemAvInfo {
  GameGeometry geometry{};
  SystemTiming timing{};
};

using Environment = bool(*)(unsigned, void*);
using VideoRefresh = void(*)(const void*, unsigned, unsigned, std::size_t);
using AudioSample = void(*)(std::int16_t, std::int16_t);
using AudioBatch = std::size_t(*)(const std::int16_t*, std::size_t);
using InputPoll = void(*)();
using InputState = std::int16_t(*)(unsigned, unsigned, unsigned, unsigned);

struct StaticApi {
  void (*set_environment)(Environment){};
  void (*set_video_refresh)(VideoRefresh){};
  void (*set_audio_sample)(AudioSample){};
  void (*set_audio_sample_batch)(AudioBatch){};
  void (*set_input_poll)(InputPoll){};
  void (*set_input_state)(InputState){};
  void (*init)(){};
  void (*deinit)(){};
  bool (*load_game)(const GameInfo*){};
  void (*unload_game)(){};
  void (*get_system_av_info)(SystemAvInfo*){};
  void (*run)(){};
  void (*reset)(){};
  std::size_t (*serialize_size)(){};
  bool (*serialize)(void*, std::size_t){};
  bool (*unserialize)(const void*, std::size_t){};
};

} // namespace corehost::lr
