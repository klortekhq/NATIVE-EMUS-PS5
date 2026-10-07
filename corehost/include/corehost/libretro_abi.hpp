#pragma once
#include <cstddef>
#include <cstdint>

namespace corehost::lr {

constexpr unsigned api_version = 1;
constexpr unsigned environment_experimental = 0x10000;

constexpr unsigned env_set_message = 6;
constexpr unsigned env_get_system_directory = 9;
constexpr unsigned env_set_pixel_format = 10;
constexpr unsigned env_get_variable = 15;
constexpr unsigned env_set_variables = 16;
constexpr unsigned env_get_variable_update = 17;
constexpr unsigned env_get_log_interface = 27;
constexpr unsigned env_get_save_directory = 31;
constexpr unsigned env_set_system_av_info = 32;
constexpr unsigned env_set_geometry = 37;
constexpr unsigned env_get_language = 39;
constexpr unsigned env_get_vfs_interface = 45 | environment_experimental;
constexpr unsigned env_get_audio_video_enable = 47 | environment_experimental;
constexpr unsigned env_get_input_bitmasks = 51 | environment_experimental;
constexpr unsigned env_get_core_options_version = 52;
constexpr unsigned env_get_message_interface_version = 59;
constexpr unsigned env_set_variable = 70;
constexpr unsigned env_get_savestate_context = 72 | environment_experimental;
constexpr unsigned env_get_target_sample_rate = 81 | environment_experimental;

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

constexpr unsigned memory_save_ram = 0;
constexpr unsigned language_english = 0;
constexpr unsigned av_enable_video = 1u << 0;
constexpr unsigned av_enable_audio = 1u << 1;
constexpr unsigned savestate_context_normal = 0;

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
  const char* key{};
  const char* value{};
};

struct SystemInfo {
  const char* library_name{};
  const char* library_version{};
  const char* valid_extensions{};
  bool need_fullpath{};
  bool block_extract{};
};

struct Message {
  const char* msg{};
  unsigned frames{};
};

struct GameInfo {
  const char* path{};
  const void* data{};
  std::size_t size{};
  const char* meta{};
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

// Libretro VFS API v1. Later interface fields are intentionally not advertised
// until they are implemented. Beetle PSX HW's hybrid VFS falls back to v1 for
// URI-backed reads, which is sufficient for open/seek/read of disc images.
struct VfsFileHandle;

constexpr unsigned vfs_file_access_read = 1u << 0;
constexpr unsigned vfs_file_access_write = 1u << 1;
constexpr unsigned vfs_file_access_read_write =
    vfs_file_access_read | vfs_file_access_write;
constexpr int vfs_seek_start = 0;
constexpr int vfs_seek_current = 1;
constexpr int vfs_seek_end = 2;

using VfsGetPath = const char*(*)(VfsFileHandle*);
using VfsOpen = VfsFileHandle*(*)(const char*, unsigned, unsigned);
using VfsClose = int(*)(VfsFileHandle*);
using VfsSize = std::int64_t(*)(VfsFileHandle*);
using VfsTell = std::int64_t(*)(VfsFileHandle*);
using VfsSeek = std::int64_t(*)(VfsFileHandle*, std::int64_t, int);
using VfsRead = std::int64_t(*)(VfsFileHandle*, void*, std::uint64_t);
using VfsWrite = std::int64_t(*)(VfsFileHandle*, const void*, std::uint64_t);
using VfsFlush = int(*)(VfsFileHandle*);
using VfsRemove = int(*)(const char*);
using VfsRename = int(*)(const char*, const char*);

struct VfsInterface {
  // v1
  VfsGetPath get_path{};
  VfsOpen open{};
  VfsClose close{};
  VfsSize size{};
  VfsTell tell{};
  VfsSeek seek{};
  VfsRead read{};
  VfsWrite write{};
  VfsFlush flush{};
  VfsRemove remove{};
  VfsRename rename{};

  // v2-v5 placeholders. Keeping the complete pointer-sized tail makes this
  // object ABI-safe if a core compiled with a newer libretro.h inspects its
  // struct size, while we still reject requests newer than v1.
  void* truncate{};
  void* stat{};
  void* mkdir{};
  void* opendir{};
  void* readdir{};
  void* dirent_get_name{};
  void* dirent_is_dir{};
  void* closedir{};
  void* stat_64{};
  void* set_readonly{};
  void* get_mtime{};
  void* set_mtime{};
  void* copy_begin{};
  void* copy_step{};
  void* copy_close{};
  void* dirent_stat{};
};

struct VfsInterfaceInfo {
  std::uint32_t required_interface_version{};
  VfsInterface* iface{};
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
  void (*get_system_info)(SystemInfo*){};
  void (*set_controller_port_device)(unsigned, unsigned){};
  bool (*load_game)(const GameInfo*){};
  void (*unload_game)(){};
  void (*get_system_av_info)(SystemAvInfo*){};
  void (*run)(){};
  void (*reset)(){};
  std::size_t (*serialize_size)(){};
  bool (*serialize)(void*, std::size_t){};
  bool (*unserialize)(const void*, std::size_t){};
  void* (*get_memory_data)(unsigned){};
  std::size_t (*get_memory_size)(unsigned){};
};

} // namespace corehost::lr
