#include "native_libretro_host.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

extern "C" std::int32_t sceKernelUsleep(std::uint32_t microseconds);

namespace native_emu::libretro {
namespace {

NativeHost* g_active = nullptr;

bool pressed(const ps5rt::ControllerState& pad, ps5rt::Button button) noexcept {
  return (pad.buttons & static_cast<std::uint32_t>(button)) != 0;
}

std::string option_default(const char* definition) {
  if (!definition)
    return {};
  std::string_view text(definition);
  const auto semicolon = text.find(';');
  if (semicolon == std::string_view::npos)
    return {};
  text.remove_prefix(semicolon + 1);
  while (!text.empty() && text.front() == ' ')
    text.remove_prefix(1);
  const auto pipe = text.find('|');
  if (pipe != std::string_view::npos)
    text = text.substr(0, pipe);
  return std::string(text);
}

ps5rt::PixelFormat pixel_format(retro_pixel_format fmt) noexcept {
  switch (fmt) {
    case RETRO_PIXEL_FORMAT_RGB565: return ps5rt::PixelFormat::rgb565;
    case RETRO_PIXEL_FORMAT_XRGB8888: return ps5rt::PixelFormat::xrgb8888;
    case RETRO_PIXEL_FORMAT_0RGB1555:
    default: return ps5rt::PixelFormat::rgb1555;
  }
}

std::uint16_t joypad_mask(const ps5rt::ControllerState& pad) noexcept {
  std::uint16_t mask = 0;
  auto set = [&](unsigned id, bool on) {
    if (on && id < 16)
      mask |= static_cast<std::uint16_t>(1u << id);
  };
  set(RETRO_DEVICE_ID_JOYPAD_B, pressed(pad, ps5rt::Button::square));
  set(RETRO_DEVICE_ID_JOYPAD_Y, pressed(pad, ps5rt::Button::triangle));
  set(RETRO_DEVICE_ID_JOYPAD_SELECT, pressed(pad, ps5rt::Button::touchpad));
  set(RETRO_DEVICE_ID_JOYPAD_START, pressed(pad, ps5rt::Button::options));
  set(RETRO_DEVICE_ID_JOYPAD_UP, pressed(pad, ps5rt::Button::up));
  set(RETRO_DEVICE_ID_JOYPAD_DOWN, pressed(pad, ps5rt::Button::down));
  set(RETRO_DEVICE_ID_JOYPAD_LEFT, pressed(pad, ps5rt::Button::left));
  set(RETRO_DEVICE_ID_JOYPAD_RIGHT, pressed(pad, ps5rt::Button::right));
  set(RETRO_DEVICE_ID_JOYPAD_A, pressed(pad, ps5rt::Button::cross));
  set(RETRO_DEVICE_ID_JOYPAD_X, pressed(pad, ps5rt::Button::circle));
  set(RETRO_DEVICE_ID_JOYPAD_L, pressed(pad, ps5rt::Button::l1));
  set(RETRO_DEVICE_ID_JOYPAD_R, pressed(pad, ps5rt::Button::r1));
  set(RETRO_DEVICE_ID_JOYPAD_L2, pad.l2 > 0.5f);
  set(RETRO_DEVICE_ID_JOYPAD_R2, pad.r2 > 0.5f);
  set(RETRO_DEVICE_ID_JOYPAD_L3, pressed(pad, ps5rt::Button::l3));
  set(RETRO_DEVICE_ID_JOYPAD_R3, pressed(pad, ps5rt::Button::r3));
  return mask;
}

} // namespace

NativeHost::NativeHost(CoreApi api, Paths paths)
    : api_(api), paths_(std::move(paths)) {}

NativeHost::~NativeHost() {
  stop();
}

ps5rt::Result NativeHost::start() noexcept {
  stop();
  error_.clear();

  if (!api_.set_environment || !api_.set_video_refresh ||
      !api_.set_audio_sample || !api_.set_audio_sample_batch ||
      !api_.set_input_poll || !api_.set_input_state ||
      !api_.init || !api_.deinit || !api_.get_system_info ||
      !api_.get_system_av_info || !api_.load_game ||
      !api_.unload_game || !api_.run) {
    error_ = "incomplete static libretro API";
    return {ps5rt::ErrorCode::invalid_argument, 0, "incomplete static libretro API"};
  }

  if (g_active && g_active != this) {
    error_ = "another static libretro core is already active";
    return {ps5rt::ErrorCode::system_error, 0, "another static libretro core is active"};
  }
  g_active = this;

  api_.set_environment(&NativeHost::environment_cb);
  api_.set_video_refresh(&NativeHost::video_cb);
  api_.set_audio_sample(&NativeHost::audio_sample_cb);
  api_.set_audio_sample_batch(&NativeHost::audio_batch_cb);
  api_.set_input_poll(&NativeHost::input_poll_cb);
  api_.set_input_state(&NativeHost::input_state_cb);

  std::memset(&system_info_, 0, sizeof(system_info_));
  api_.get_system_info(&system_info_);

  api_.init();
  initialized_ = true;

  if (api_.set_controller_port_device)
    api_.set_controller_port_device(0, RETRO_DEVICE_JOYPAD);

  retro_game_info game{};
  if (!load_content(game)) {
    stop();
    return {ps5rt::ErrorCode::io_error, 0, "failed to prepare content"};
  }
  if (!api_.load_game(&game)) {
    error_ = "retro_load_game rejected content";
    stop();
    return {ps5rt::ErrorCode::io_error, 0, "retro_load_game rejected content"};
  }
  loaded_ = true;

  std::memset(&av_info_, 0, sizeof(av_info_));
  api_.get_system_av_info(&av_info_);

  auto vr = video_.open();
  if (!vr) {
    error_ = std::string(vr.message);
    stop();
    return vr;
  }

  ps5rt::AudioSpec spec{};
  spec.sample_rate = static_cast<std::uint32_t>(
      av_info_.timing.sample_rate > 1.0 ? av_info_.timing.sample_rate : 48000.0);
  spec.channels = 2;
  spec.frames_per_grain = 256;
  spec.format = ps5rt::SampleFormat::s16;
  auto ar = audio_.open(spec);
  if (!ar) {
    error_ = std::string(ar.message);
    stop();
    return ar;
  }

  input_ready_ = static_cast<bool>(ps5rt::initialize_input());
  load_sram();

  running_ = true;
  quit_ = false;
  frames_ = 0;
  return ps5rt::Result::success();
}

bool NativeHost::run_frame() noexcept {
  if (!running_ || quit_ || !loaded_)
    return false;

  api_.run();
  ++frames_;

  if ((frames_ % 300u) == 0u)
    save_sram();

  // VideoOut blocks on a 60 Hz vblank. Add only the missing delay for
  // 50 Hz/PAL-style cores; 60 Hz/NTSC cores need no extra throttle here.
  if (av_info_.timing.fps > 1.0 && av_info_.timing.fps < 55.0) {
    const double target_us = 1000000.0 / av_info_.timing.fps;
    if (target_us > 16667.0)
      (void)sceKernelUsleep(static_cast<std::uint32_t>(target_us - 16667.0));
  }

  return !quit_;
}

void NativeHost::stop() noexcept {
  if (loaded_)
    save_sram();

  audio_.close();
  video_.close();
  if (input_ready_) {
    ps5rt::shutdown_input();
    input_ready_ = false;
  }

  if (loaded_ && api_.unload_game) {
    api_.unload_game();
    loaded_ = false;
  }
  if (initialized_ && api_.deinit) {
    api_.deinit();
    initialized_ = false;
  }

  content_buffer_.clear();
  running_ = false;
  quit_ = false;
  frames_ = 0;
  if (g_active == this)
    g_active = nullptr;
}

bool NativeHost::environment_cb(unsigned cmd, void* data) {
  return g_active && g_active->environment(cmd, data);
}

void NativeHost::video_cb(
    const void* data, unsigned width, unsigned height, std::size_t pitch) {
  if (g_active)
    g_active->video(data, width, height, pitch);
}

void NativeHost::audio_sample_cb(std::int16_t left, std::int16_t right) {
  if (!g_active)
    return;
  const std::int16_t pair[2]{left, right};
  (void)g_active->audio(pair, 1);
}

std::size_t NativeHost::audio_batch_cb(
    const std::int16_t* data, std::size_t frames) {
  return g_active ? g_active->audio(data, frames) : 0;
}

void NativeHost::input_poll_cb() {
  if (g_active)
    g_active->poll_input();
}

std::int16_t NativeHost::input_state_cb(
    unsigned port, unsigned device, unsigned index, unsigned id) {
  return g_active ? g_active->input_state(port, device, index, id) : 0;
}

void NativeHost::log_cb(enum retro_log_level, const char* fmt, ...) {
  if (!fmt)
    return;
  va_list args;
  va_start(args, fmt);
  std::vfprintf(stderr, fmt, args);
  va_end(args);
}

bool NativeHost::environment(unsigned cmd, void* data) noexcept {
  switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
      if (!data) return false;
      *static_cast<const char**>(data) = paths_.system_dir.c_str();
      return true;

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
      if (!data) return false;
      *static_cast<const char**>(data) = paths_.save_dir.c_str();
      return true;

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
      if (!data) return false;
      const auto fmt = *static_cast<const retro_pixel_format*>(data);
      if (fmt != RETRO_PIXEL_FORMAT_0RGB1555 &&
          fmt != RETRO_PIXEL_FORMAT_RGB565 &&
          fmt != RETRO_PIXEL_FORMAT_XRGB8888)
        return false;
      pixel_format_ = fmt;
      return true;
    }

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
      if (!data) return false;
      static_cast<retro_log_callback*>(data)->log = &NativeHost::log_cb;
      return true;

    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
      return true;

    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:
      if (!data) return true;
      *static_cast<retro_av_enable_flags*>(data) =
          static_cast<retro_av_enable_flags>(
              RETRO_AV_ENABLE_VIDEO | RETRO_AV_ENABLE_AUDIO);
      return true;

    case RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT:
      if (!data) return true;
      *static_cast<retro_savestate_context*>(data) =
          RETRO_SAVESTATE_CONTEXT_NORMAL;
      return true;

    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 0;
      return true;

    case RETRO_ENVIRONMENT_SET_VARIABLES: {
      if (!data) return false;
      const auto* vars = static_cast<const retro_variable*>(data);
      for (; vars->key; ++vars) {
        const auto value = option_default(vars->value);
        if (!value.empty())
          variables_[vars->key] = value;
      }
      return true;
    }

    case RETRO_ENVIRONMENT_GET_VARIABLE: {
      if (!data) return false;
      auto* var = static_cast<retro_variable*>(data);
      if (!var->key) return false;
      const auto it = variables_.find(var->key);
      if (it == variables_.end()) {
        var->value = nullptr;
        return false;
      }
      var->value = it->second.c_str();
      return true;
    }

    case RETRO_ENVIRONMENT_SET_VARIABLE: {
      if (!data) return false;
      const auto* var = static_cast<const retro_variable*>(data);
      if (!var->key || !var->value) return false;
      variables_[var->key] = var->value;
      return true;
    }

    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
      if (!data) return false;
      *static_cast<bool*>(data) = false;
      return true;

    case RETRO_ENVIRONMENT_GET_LANGUAGE:
      if (!data) return false;
      *static_cast<unsigned*>(data) = RETRO_LANGUAGE_ENGLISH;
      return true;

    case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 0;
      return true;

    case RETRO_ENVIRONMENT_GET_TARGET_SAMPLE_RATE:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 48000;
      return true;

    case RETRO_ENVIRONMENT_SET_GEOMETRY:
      if (!data) return false;
      av_info_.geometry = *static_cast<const retro_game_geometry*>(data);
      return true;

    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
      if (!data) return false;
      av_info_ = *static_cast<const retro_system_av_info*>(data);
      return true;

    case RETRO_ENVIRONMENT_SET_MESSAGE:
      if (data && static_cast<const retro_message*>(data)->msg)
        std::fprintf(stderr, "core: %s\n", static_cast<const retro_message*>(data)->msg);
      return true;

#ifdef RETRO_ENVIRONMENT_SET_MESSAGE_EXT
    case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
      if (data && static_cast<const retro_message_ext*>(data)->msg)
        std::fprintf(stderr, "core: %s\n", static_cast<const retro_message_ext*>(data)->msg);
      return true;
#endif

    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
    case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
    case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
      return true;

    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK:
    case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
    case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK:
    case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
    case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
    case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
    case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
    case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
      return false;

    default:
      return false;
  }
}

void NativeHost::video(
    const void* data, unsigned width, unsigned height, std::size_t pitch) noexcept {
  if (!data || !video_.is_open())
    return;
  ps5rt::VideoFrame frame{};
  frame.pixels = data;
  frame.width = width;
  frame.height = height;
  frame.pitch_bytes = static_cast<std::uint32_t>(pitch);
  frame.format = pixel_format(pixel_format_);
  frame.display_aspect =
      av_info_.geometry.aspect_ratio > 0.0f
          ? av_info_.geometry.aspect_ratio
          : (height ? static_cast<float>(width) / static_cast<float>(height) : 0.0f);
  const auto result = video_.present(frame);
  if (!result) {
    error_ = std::string(result.message);
    quit_ = true;
  }
}

std::size_t NativeHost::audio(
    const std::int16_t* data, std::size_t frames) noexcept {
  if (!data || !frames || !audio_.is_open())
    return 0;
  const auto bytes = std::as_bytes(std::span(data, frames * 2));
  const auto result = audio_.write(bytes);
  if (!result) {
    error_ = std::string(result.message);
    quit_ = true;
    return 0;
  }
  return frames;
}

void NativeHost::poll_input() noexcept {
  input_ = {};
  if (input_ready_)
    (void)ps5rt::poll_input(input_);

  if (input_.controllers[0].connected &&
      pressed(input_.controllers[0], ps5rt::Button::options) &&
      pressed(input_.controllers[0], ps5rt::Button::touchpad))
    quit_ = true;
}

std::int16_t NativeHost::input_state(
    unsigned port, unsigned device, unsigned index, unsigned id) const noexcept {
  if (port >= input_.controllers.size())
    return 0;

  const auto& pad = input_.controllers[port];
  if (device == RETRO_DEVICE_JOYPAD) {
    const auto mask = joypad_mask(pad);
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
      return static_cast<std::int16_t>(mask);
    return id < 16 && (mask & (1u << id)) ? 1 : 0;
  }

  if (device == RETRO_DEVICE_ANALOG) {
    const ps5rt::Stick& stick =
        index == RETRO_DEVICE_INDEX_ANALOG_RIGHT ? pad.right : pad.left;
    const float value =
        id == RETRO_DEVICE_ID_ANALOG_Y ? stick.y : stick.x;
    return static_cast<std::int16_t>(
        std::max(-32768.0f, std::min(32767.0f, value * 32767.0f)));
  }

  if (device == RETRO_DEVICE_MOUSE && port == 0) {
    if (id == RETRO_DEVICE_ID_MOUSE_X)
      return static_cast<std::int16_t>(input_.mouse.delta_x);
    if (id == RETRO_DEVICE_ID_MOUSE_Y)
      return static_cast<std::int16_t>(input_.mouse.delta_y);
    if (id == RETRO_DEVICE_ID_MOUSE_LEFT)
      return (input_.mouse.buttons & 1u) ? 1 : 0;
    if (id == RETRO_DEVICE_ID_MOUSE_RIGHT)
      return (input_.mouse.buttons & 2u) ? 1 : 0;
  }

  return 0;
}

bool NativeHost::load_content(retro_game_info& info) noexcept {
  info = {};
  info.path = paths_.content.c_str();

  if (system_info_.need_fullpath)
    return true;

  ps5rt::RandomAccessReaderPtr reader;
  const auto opened =
      ps5rt::open_random_access(paths_.content, ps5rt::OpenMode::read_only, reader);
  if (!opened || !reader) {
    error_ = "cannot open content";
    return false;
  }

  std::uint64_t bytes = 0;
  if (!reader->size(bytes) || bytes > (512ull * 1024ull * 1024ull)) {
    error_ = "content size invalid";
    return false;
  }

  content_buffer_.resize(static_cast<std::size_t>(bytes));
  std::size_t read = 0;
  if (!reader->read_at(
          0, std::as_writable_bytes(std::span(content_buffer_)), read) ||
      read != content_buffer_.size()) {
    error_ = "content read failed";
    content_buffer_.clear();
    return false;
  }

  info.data = content_buffer_.data();
  info.size = content_buffer_.size();
  return true;
}

std::string NativeHost::save_path() const {
  std::string name = paths_.content;
  const auto slash = name.find_last_of("/\\");
  if (slash != std::string::npos)
    name.erase(0, slash + 1);
  const auto dot = name.find_last_of('.');
  if (dot != std::string::npos)
    name.erase(dot);
  if (name.empty())
    name = "game";
  return paths_.save_dir + "/" + name + ".srm";
}

void NativeHost::load_sram() noexcept {
  if (!api_.get_memory_data || !api_.get_memory_size)
    return;
  void* memory = api_.get_memory_data(RETRO_MEMORY_SAVE_RAM);
  const std::size_t size = api_.get_memory_size(RETRO_MEMORY_SAVE_RAM);
  if (!memory || !size)
    return;

  const auto path = save_path();
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f)
    return;
  if (std::fseek(f, 0, SEEK_END) == 0) {
    const long n = std::ftell(f);
    if (n == static_cast<long>(size) && std::fseek(f, 0, SEEK_SET) == 0)
      (void)std::fread(memory, 1, size, f);
  }
  std::fclose(f);
}

void NativeHost::save_sram() noexcept {
  if (!api_.get_memory_data || !api_.get_memory_size || !loaded_)
    return;
  void* memory = api_.get_memory_data(RETRO_MEMORY_SAVE_RAM);
  const std::size_t size = api_.get_memory_size(RETRO_MEMORY_SAVE_RAM);
  if (!memory || !size)
    return;

  const auto path = save_path();
  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f)
    return;
  (void)std::fwrite(memory, 1, size, f);
  std::fclose(f);
}

} // namespace native_emu::libretro
