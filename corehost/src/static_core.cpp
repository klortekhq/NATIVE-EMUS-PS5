#include <corehost/static_core.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <utility>

namespace corehost {
namespace {
std::mutex g_mutex;
StaticCore* g_active = nullptr;

StaticCore* active() noexcept { return g_active; }

std::string parse_default(std::string_view definition) {
  const auto semicolon = definition.find(';');
  if (semicolon == std::string_view::npos) return {};
  auto values = definition.substr(semicolon + 1);
  while (!values.empty() && values.front() == ' ') values.remove_prefix(1);
  const auto pipe = values.find('|');
  return std::string(values.substr(0, pipe));
}
} // namespace

bool environment_trampoline(unsigned cmd, void* data) {
  auto* core = active();
  return core ? core->environment(cmd, data) : false;
}

void video_trampoline(const void* data, unsigned w, unsigned h, std::size_t pitch) {
  if (auto* core = active()) core->video(data, w, h, pitch);
}

void audio_sample_trampoline(std::int16_t l, std::int16_t r) {
  const std::int16_t frame[2]{l, r};
  if (auto* core = active()) core->audio(frame, 1);
}

std::size_t audio_batch_trampoline(const std::int16_t* data, std::size_t frames) {
  if (auto* core = active()) return core->audio(data, frames);
  return frames;
}

void input_poll_trampoline() {
  auto* core = active();
  if (!core || !core->hooks_.input) return;
  for (unsigned port = 0; port < core->input_cache_.size(); ++port)
    core->input_cache_[port] = core->hooks_.input(port);
}

std::int16_t input_state_trampoline(unsigned port, unsigned device, unsigned index, unsigned id) {
  if (auto* core = active()) return core->input_state(port, device, index, id);
  return 0;
}

void log_trampoline(lr::LogLevel, const char* fmt, ...) {
  auto* core = active();
  if (!core || !fmt) return;

  char stack[2048];
  va_list args;
  va_start(args, fmt);
  const int required = std::vsnprintf(stack, sizeof(stack), fmt, args);
  va_end(args);

  if (required < 0) return;
  if (static_cast<std::size_t>(required) < sizeof(stack)) {
    core->log(std::string_view(stack, static_cast<std::size_t>(required)));
    return;
  }

  std::string dynamic(static_cast<std::size_t>(required) + 1u, '\0');
  va_start(args, fmt);
  std::vsnprintf(dynamic.data(), dynamic.size(), fmt, args);
  va_end(args);
  dynamic.resize(static_cast<std::size_t>(required));
  core->log(dynamic);
}

StaticCore::StaticCore(std::string name, lr::StaticApi api, HostPaths paths, Hooks hooks)
  : name_(std::move(name)), api_(api), paths_(std::move(paths)), hooks_(std::move(hooks)) {}

StaticCore::~StaticCore() { shutdown(); }

bool StaticCore::initialize(std::string& error) {
  std::scoped_lock lock(g_mutex);
  if (initialized_) return true;
  if (g_active && g_active != this) {
    error = "another static core is already active";
    return false;
  }

  if (!api_.set_environment || !api_.set_video_refresh || !api_.set_audio_sample ||
      !api_.set_audio_sample_batch || !api_.set_input_poll || !api_.set_input_state ||
      !api_.init || !api_.deinit || !api_.get_system_info || !api_.load_game ||
      !api_.unload_game || !api_.get_system_av_info || !api_.run) {
    error = "incomplete static libretro API";
    return false;
  }

  g_active = this;
  api_.set_environment(environment_trampoline);
  api_.set_video_refresh(video_trampoline);
  api_.set_audio_sample(audio_sample_trampoline);
  api_.set_audio_sample_batch(audio_batch_trampoline);
  api_.set_input_poll(input_poll_trampoline);
  api_.set_input_state(input_state_trampoline);

  api_.get_system_info(&system_info_);
  api_.init();

  if (api_.set_controller_port_device)
    api_.set_controller_port_device(0, lr::device_joypad);

  api_.get_system_av_info(&av_info_);
  initialized_ = true;
  return true;
}

bool StaticCore::load_path(std::string_view path, std::string& error) {
  if (!initialized_) {
    error = "core not initialized";
    return false;
  }
  if (path.empty()) {
    error = "empty content path";
    return false;
  }
  if (loaded_) unload();

  const std::string stable_path(path);
  lr::GameInfo info{};
  info.path = stable_path.c_str();

  if (!system_info_.need_fullpath) {
    std::FILE* f = std::fopen(stable_path.c_str(), "rb");
    if (!f) {
      error = "cannot open content";
      return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
      std::fclose(f);
      error = "cannot seek content";
      return false;
    }
    const long n = std::ftell(f);
    if (n < 0 || static_cast<unsigned long>(n) > 512ul * 1024ul * 1024ul ||
        std::fseek(f, 0, SEEK_SET) != 0) {
      std::fclose(f);
      error = "content size invalid";
      return false;
    }
    content_buffer_.resize(static_cast<std::size_t>(n));
    const auto got = content_buffer_.empty()
        ? 0u
        : std::fread(content_buffer_.data(), 1, content_buffer_.size(), f);
    std::fclose(f);
    if (got != content_buffer_.size()) {
      content_buffer_.clear();
      error = "content read failed";
      return false;
    }
    info.data = content_buffer_.empty() ? nullptr : content_buffer_.data();
    info.size = content_buffer_.size();
  }

  if (!api_.load_game(&info)) {
    content_buffer_.clear();
    error = "core rejected content";
    return false;
  }

  api_.get_system_av_info(&av_info_);
  loaded_ = true;
  return true;
}

void StaticCore::run_frame() { if (loaded_) api_.run(); }
void StaticCore::reset() { if (loaded_ && api_.reset) api_.reset(); }

void StaticCore::unload() {
  if (loaded_) {
    api_.unload_game();
    loaded_ = false;
  }
  content_buffer_.clear();
}

void StaticCore::shutdown() {
  std::scoped_lock lock(g_mutex);
  unload();
  if (initialized_) {
    api_.deinit();
    initialized_ = false;
  }
  if (g_active == this) g_active = nullptr;
}

bool StaticCore::save_state(std::vector<std::uint8_t>& out) {
  if (!loaded_ || !api_.serialize_size || !api_.serialize) return false;
  const auto size = api_.serialize_size();
  if (!size) return false;
  out.resize(size);
  if (!api_.serialize(out.data(), out.size())) {
    out.clear();
    return false;
  }
  return true;
}

bool StaticCore::load_state(const void* data, std::size_t size) {
  return loaded_ && api_.unserialize && api_.unserialize(data, size);
}

void StaticCore::set_option(std::string key, std::string value) {
  auto it = options_.find(key);
  if (it != options_.end() && it->second == value) return;
  options_[std::move(key)] = std::move(value);
  options_dirty_ = true;
}

void* StaticCore::memory_data(unsigned id) noexcept {
  return api_.get_memory_data ? api_.get_memory_data(id) : nullptr;
}

std::size_t StaticCore::memory_size(unsigned id) const noexcept {
  return api_.get_memory_size ? api_.get_memory_size(id) : 0;
}

void StaticCore::register_variables(const lr::Variable* vars) {
  if (!vars) return;
  for (auto* v = vars; v->key; ++v) {
    const auto def = parse_default(v->value ? std::string_view(v->value) : std::string_view{});
    option_defaults_[v->key] = def;
    if (!options_.contains(v->key)) options_[v->key] = def;
  }
}

bool StaticCore::environment(unsigned cmd, void* data) {
  switch (cmd) {
    case lr::env_set_message:
      if (data && static_cast<const lr::Message*>(data)->msg)
        log(static_cast<const lr::Message*>(data)->msg);
      return true;

    case lr::env_get_system_directory:
      if (!data) return false;
      *static_cast<const char**>(data) = paths_.system_dir.c_str();
      return true;

    case lr::env_get_save_directory:
      if (!data) return false;
      *static_cast<const char**>(data) = paths_.save_dir.c_str();
      return true;

    case lr::env_set_pixel_format:
      if (!data) return false;
      pixel_format_ = *static_cast<const lr::PixelFormat*>(data);
      return pixel_format_ == lr::PixelFormat::xrgb1555 ||
             pixel_format_ == lr::PixelFormat::xrgb8888 ||
             pixel_format_ == lr::PixelFormat::rgb565;

    case lr::env_set_variables:
      register_variables(static_cast<const lr::Variable*>(data));
      return true;

    case lr::env_get_variable: {
      if (!data) return false;
      auto& v = *static_cast<lr::Variable*>(data);
      if (!v.key) { v.value = nullptr; return true; }
      auto it = options_.find(v.key);
      v.value = it == options_.end() ? nullptr : it->second.c_str();
      return true;
    }

    case lr::env_set_variable: {
      if (!data) return false;
      const auto& v = *static_cast<const lr::Variable*>(data);
      if (!v.key || !v.value) return false;
      set_option(v.key, v.value);
      return true;
    }

    case lr::env_get_variable_update:
      if (!data) return false;
      *static_cast<bool*>(data) = options_dirty_;
      options_dirty_ = false;
      return true;

    case lr::env_get_log_interface:
      if (!data) return false;
      static_cast<lr::LogCallback*>(data)->log = log_trampoline;
      return true;

    case lr::env_set_system_av_info:
      if (!data) return false;
      av_info_ = *static_cast<const lr::SystemAvInfo*>(data);
      return true;

    case lr::env_set_geometry:
      if (!data) return false;
      av_info_.geometry = *static_cast<const lr::GameGeometry*>(data);
      return true;

    case lr::env_get_language:
      if (!data) return false;
      *static_cast<unsigned*>(data) = lr::language_english;
      return true;

    case lr::env_get_audio_video_enable:
      if (!data) return false;
      *static_cast<unsigned*>(data) = lr::av_enable_video | lr::av_enable_audio;
      return true;

    case lr::env_get_input_bitmasks:
      if (!data) return false;
      *static_cast<bool*>(data) = true;
      return true;

    case lr::env_get_core_options_version:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 0;
      return true;

    case lr::env_get_message_interface_version:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 0;
      return true;

    case lr::env_get_savestate_context:
      if (!data) return false;
      *static_cast<unsigned*>(data) = lr::savestate_context_normal;
      return true;

    case lr::env_get_target_sample_rate:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 48000;
      return true;

    case lr::env_get_vfs_interface:
      return false;

    default:
      return hooks_.environment ? hooks_.environment(cmd, data) : false;
  }
}

void StaticCore::video(const void* data, unsigned width, unsigned height, std::size_t pitch) {
  if (hooks_.video) hooks_.video(data, width, height, pitch, pixel_format_);
}

std::size_t StaticCore::audio(const std::int16_t* data, std::size_t frames) {
  return hooks_.audio_batch ? hooks_.audio_batch(data, frames) : frames;
}

std::int16_t StaticCore::input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
  if (port >= input_cache_.size()) return 0;
  const auto& state = input_cache_[port];

  if (device == lr::device_joypad) {
    if (id == lr::device_joypad_mask) return static_cast<std::int16_t>(state.joypad_mask);
    if (id < 16) return (state.joypad_mask & (std::uint16_t{1} << id)) ? 1 : 0;
    return 0;
  }

  if (device == lr::device_analog) {
    if (index == lr::analog_left) return id == lr::analog_x ? state.left_x : state.left_y;
    if (index == lr::analog_right) return id == lr::analog_x ? state.right_x : state.right_y;
    if (index == lr::analog_button) {
      if (id == 12) return state.l2;
      if (id == 13) return state.r2;
    }
    return 0;
  }

  if (device == lr::device_keyboard)
    return state.key_down(id) ? 1 : 0;

  if (device == lr::device_mouse) {
    switch (id) {
      case lr::mouse_x: return state.mouse_x;
      case lr::mouse_y: return state.mouse_y;
      case lr::mouse_left: return (state.mouse_buttons & (1u << 0)) ? 1 : 0;
      case lr::mouse_right: return (state.mouse_buttons & (1u << 1)) ? 1 : 0;
      case lr::mouse_middle: return (state.mouse_buttons & (1u << 2)) ? 1 : 0;
      case lr::mouse_button_4: return (state.mouse_buttons & (1u << 3)) ? 1 : 0;
      case lr::mouse_button_5: return (state.mouse_buttons & (1u << 4)) ? 1 : 0;
      case lr::mouse_wheel_up: return state.mouse_wheel_y > 0 ? 1 : 0;
      case lr::mouse_wheel_down: return state.mouse_wheel_y < 0 ? 1 : 0;
      case lr::mouse_wheel_left: return state.mouse_wheel_x < 0 ? 1 : 0;
      case lr::mouse_wheel_right: return state.mouse_wheel_x > 0 ? 1 : 0;
      default: return 0;
    }
  }

  return 0;
}

void StaticCore::log(std::string_view message) {
  if (hooks_.log) hooks_.log(message);
}

} // namespace corehost
