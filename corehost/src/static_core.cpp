#include <corehost/static_core.hpp>

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
      !api_.init || !api_.deinit || !api_.load_game || !api_.unload_game || !api_.run) {
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
  api_.init();
  initialized_ = true;
  return true;
}

bool StaticCore::load_path(std::string_view path, std::string& error) {
  if (!initialized_) {
    error = "core not initialized";
    return false;
  }
  if (loaded_) unload();
  const std::string stable_path(path);
  lr::GameInfo info{stable_path.c_str(), nullptr, 0, nullptr};
  if (!api_.load_game(&info)) {
    error = "core rejected content";
    return false;
  }
  loaded_ = true;
  return true;
}

void StaticCore::run_frame() { if (loaded_) api_.run(); }
void StaticCore::reset() { if (loaded_ && api_.reset) api_.reset(); }
void StaticCore::unload() { if (loaded_) { api_.unload_game(); loaded_ = false; } }

void StaticCore::shutdown() {
  std::scoped_lock lock(g_mutex);
  if (loaded_) { api_.unload_game(); loaded_ = false; }
  if (initialized_) { api_.deinit(); initialized_ = false; }
  if (g_active == this) g_active = nullptr;
}

bool StaticCore::save_state(std::vector<std::uint8_t>& out) {
  if (!loaded_ || !api_.serialize_size || !api_.serialize) return false;
  const auto size = api_.serialize_size();
  if (!size) return false;
  out.resize(size);
  if (!api_.serialize(out.data(), out.size())) { out.clear(); return false; }
  return true;
}

bool StaticCore::load_state(const void* data, std::size_t size) {
  return loaded_ && api_.unserialize && api_.unserialize(data, size);
}

void StaticCore::set_option(std::string key, std::string value) {
  options_[std::move(key)] = std::move(value);
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
    case lr::env_get_variable_update:
      if (!data) return false;
      *static_cast<bool*>(data) = false;
      return true;
    case lr::env_get_input_bitmasks:
      if (!data) return false;
      *static_cast<bool*>(data) = true;
      return true;
    case lr::env_get_core_options_version:
      if (!data) return false;
      *static_cast<unsigned*>(data) = 0;
      return true;
    default:
      return false;
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
  }
  return 0;
}

} // namespace corehost
