#include <corehost/ps5_runner.hpp>

#include <corehost/ps5rt_bridge.hpp>
#include <corehost/static_core.hpp>

#include <ps5rt/app.hpp>
#include <ps5rt/audio.hpp>
#include <ps5rt/input.hpp>
#include <ps5rt/lifecycle.hpp>
#include <ps5rt/vfs.hpp>
#include <ps5rt/video.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>

extern "C" std::int32_t sceKernelUsleep(std::uint32_t microseconds);

namespace corehost {
namespace {

bool file_exists(const std::string& path) noexcept {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::fclose(f);
  return true;
}

std::string read_path_file(const std::string& path) {
  char value[2048]{};
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return {};
  if (!std::fgets(value, sizeof(value), f))
    value[0] = '\0';
  std::fclose(f);
  value[std::strcspn(value, "\r\n")] = '\0';
  return value;
}

std::string pick_content(
    const std::string& root,
    const std::string& system_key,
    const char* default_content) {
  for (const auto& path_file : {
           root + "/boot.txt",
           root + "/rom-path.txt",
           std::string("/app0/boot.txt"),
           std::string("/app0/rom-path.txt")}) {
    const auto selected = read_path_file(path_file);
    if (!selected.empty() && file_exists(selected))
      return selected;
  }

  const std::string filename =
      (default_content && *default_content) ? default_content : "game.rom";
  const std::string relative =
      std::string("NATIVE-EMUS-PS5/") + system_key + "/" + filename;

  for (const auto& candidate : {
           root + "/" + filename,
           std::string("/mnt/usb0/") + relative,
           std::string("/mnt/usb1/") + relative,
           std::string("/app0/") + filename}) {
    if (file_exists(candidate))
      return candidate;
  }

  return root + "/" + filename;
}

std::string save_path(const std::string& content, const std::string& save_dir) {
  std::string name = content;
  const auto slash = name.find_last_of("/\\");
  if (slash != std::string::npos)
    name.erase(0, slash + 1);
  const auto dot = name.find_last_of('.');
  if (dot != std::string::npos)
    name.erase(dot);
  if (name.empty())
    name = "game";
  return save_dir + "/" + name + ".srm";
}

void load_sram(StaticCore& core, const std::string& path) noexcept {
  void* memory = core.memory_data(lr::memory_save_ram);
  const std::size_t size = core.memory_size(lr::memory_save_ram);
  if (!memory || !size) return;

  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return;
  if (std::fseek(f, 0, SEEK_END) == 0) {
    const long n = std::ftell(f);
    if (n == static_cast<long>(size) && std::fseek(f, 0, SEEK_SET) == 0)
      (void)std::fread(memory, 1, size, f);
  }
  std::fclose(f);
}

void save_sram(StaticCore& core, const std::string& path) noexcept {
  void* memory = core.memory_data(lr::memory_save_ram);
  const std::size_t size = core.memory_size(lr::memory_save_ram);
  if (!memory || !size) return;

  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return;
  (void)std::fwrite(memory, 1, size, f);
  std::fclose(f);
}

bool pressed(const ps5rt::ControllerState& pad, ps5rt::Button button) noexcept {
  return (pad.buttons & static_cast<std::uint32_t>(button)) != 0;
}

void log_result(const char* prefix, const ps5rt::Result& result) noexcept {
  std::fprintf(
      stderr, "%s: %.*s (native=%d)\n",
      prefix,
      static_cast<int>(result.message.size()),
      result.message.data(),
      result.native_code);
}

} // namespace

int run_linked_core_ps5(
    const Ps5RunnerConfig& config,
    lr::StaticApi api) noexcept {
  if (!config.app_name || !config.system_key || !config.data_root)
    return 2;

  const std::string base = config.data_root;
  const std::string root = base + "/" + config.system_key;
  const std::string system_dir = root + "/system";
  const std::string save_dir = root + "/saves";
  const std::string log_dir = root + "/logs";

  for (const auto& dir : {base, root, system_dir, save_dir, log_dir}) {
    const auto result = ps5rt::ensure_directory(dir);
    if (!result) {
      log_result("directory init failed", result);
      return 3;
    }
  }

  const std::string log_path = log_dir + "/native.log";
  (void)std::freopen(log_path.c_str(), "a", stderr);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  std::fprintf(stderr, "\n=== %s start ===\n", config.app_name);

  ps5rt::ShutdownStack shutdown;

  ps5rt::AppInfo app{};
  const ps5rt::AppConfig app_config{
      config.app_name,
      base,
      true,
      false,
      true,
  };
  const auto app_result = ps5rt::initialize_app(app_config, app);
  if (!app_result) {
    log_result("app init failed", app_result);
    return 10;
  }
  if (!shutdown.push(
          [](void*) noexcept {
            ps5rt::shutdown_app();
          })) {
    ps5rt::shutdown_app();
    return 10;
  }

  ps5rt::VideoDevice video;
  const auto video_result = video.open();
  if (!video_result) {
    log_result("video init failed", video_result);
    return 11;
  }
  if (!shutdown.push(
          [](void* raw) noexcept {
            static_cast<ps5rt::VideoDevice*>(raw)->close();
          },
          &video)) {
    video.close();
    return 11;
  }

  const auto input_result = ps5rt::initialize_input();
  if (!input_result) {
    log_result("input init failed", input_result);
    return 12;
  }
  if (!shutdown.push(
          [](void*) noexcept {
            ps5rt::shutdown_input();
          })) {
    ps5rt::shutdown_input();
    return 12;
  }

  ps5rt::AudioDevice audio;
  ps5rt::InputSnapshot snapshot{};
  std::string runtime_error;
  bool quit = false;
  float display_aspect = 0.0f;

  Hooks hooks{};
  hooks.video = [&](const void* data, unsigned width, unsigned height,
                    std::size_t pitch, lr::PixelFormat format) {
    if (!data || quit) return;

    ps5rt::VideoFrame frame{};
    frame.pixels = data;
    frame.width = width;
    frame.height = height;
    frame.pitch_bytes = static_cast<std::uint32_t>(pitch);
    frame.format = translate_pixel_format(format);
    frame.display_aspect = display_aspect;

    const auto result = video.present(frame);
    if (!result) {
      runtime_error = std::string(result.message);
      quit = true;
    }
  };

  hooks.audio_batch = [&](const std::int16_t* data, std::size_t frames) {
    if (!data || !frames || !audio.is_open() || quit)
      return std::size_t{0};

    const auto bytes = std::as_bytes(std::span(data, frames * 2));
    const auto result = audio.write(bytes);
    if (!result) {
      runtime_error = std::string(result.message);
      quit = true;
      return std::size_t{0};
    }
    return frames;
  };

  hooks.input = [&](unsigned port) {
    if (port == 0) {
      snapshot = {};
      const auto result = ps5rt::poll_input(snapshot);
      if (!result) {
        runtime_error = std::string(result.message);
        quit = true;
      }

      const auto& pad = snapshot.controllers[0];
      if (pad.connected &&
          pressed(pad, ps5rt::Button::options) &&
          pressed(pad, ps5rt::Button::touchpad)) {
        quit = true;
      }
    }
    return translate_ps5rt_input(snapshot, port);
  };

  hooks.log = [](std::string_view message) {
    std::fprintf(stderr, "core: %.*s", static_cast<int>(message.size()), message.data());
    if (message.empty() || message.back() != '\n')
      std::fputc('\n', stderr);
  };

  StaticCore core(
      config.app_name,
      api,
      {system_dir, save_dir},
      std::move(hooks));

  std::string error;
  if (!core.initialize(error)) {
    std::fprintf(stderr, "core init failed: %s\n", error.c_str());
    return 20;
  }

  const std::string content =
      pick_content(root, config.system_key, config.default_content);
  std::fprintf(stderr, "content=%s\n", content.c_str());

  if (!core.load_path(content, error)) {
    std::fprintf(stderr, "content load failed: %s\n", error.c_str());
    core.shutdown();
    return 21;
  }

  const auto& av = core.av_info();
  display_aspect = av.geometry.aspect_ratio > 0.0f
      ? av.geometry.aspect_ratio
      : (av.geometry.base_height
             ? static_cast<float>(av.geometry.base_width) /
                   static_cast<float>(av.geometry.base_height)
             : 0.0f);

  ps5rt::AudioSpec audio_spec{};
  const double source_rate =
      av.timing.sample_rate > 1000.0 ? av.timing.sample_rate : 48000.0;
  audio_spec.sample_rate = static_cast<std::uint32_t>(source_rate + 0.5);
  audio_spec.channels = 2;
  audio_spec.frames_per_grain = 256;
  audio_spec.format = ps5rt::SampleFormat::s16;

  const auto audio_result = audio.open(audio_spec);
  if (!audio_result) {
    log_result("audio init failed", audio_result);
    core.shutdown();
    return 22;
  }

  // Push audio before the core so LIFO teardown exactly preserves the
  // established order: core -> audio -> input -> video -> app.
  if (!shutdown.push(
          [](void* raw) noexcept {
            static_cast<ps5rt::AudioDevice*>(raw)->close();
          },
          &audio)) {
    audio.close();
    core.shutdown();
    return 22;
  }
  if (!shutdown.push(
          [](void* raw) noexcept {
            static_cast<StaticCore*>(raw)->shutdown();
          },
          &core)) {
    core.shutdown();
    return 22;
  }

  const std::string sram = save_path(content, save_dir);
  load_sram(core, sram);

  std::uint64_t frames = 0;
  while (!quit) {
    core.run_frame();
    ++frames;

    if ((frames % 300u) == 0u)
      save_sram(core, sram);

    // VideoOut naturally paces ~60 Hz. PAL-like cores need the extra gap.
    if (av.timing.fps > 1.0 && av.timing.fps < 55.0) {
      const double target_us = 1000000.0 / av.timing.fps;
      if (target_us > 16667.0)
        (void)sceKernelUsleep(
            static_cast<std::uint32_t>(target_us - 16667.0));
    }
  }

  save_sram(core, sram);
  shutdown.run();

  if (!runtime_error.empty()) {
    std::fprintf(stderr, "runtime error: %s\n", runtime_error.c_str());
    return 30;
  }

  std::fprintf(stderr, "clean exit after %llu frames\n",
               static_cast<unsigned long long>(frames));
  return 0;
}

} // namespace corehost
