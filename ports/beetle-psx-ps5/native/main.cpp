#include "content.hpp"
#include "runtime_io.hpp"
#include "vulkan_environment.hpp"
#include "vulkan_presenter.hpp"
#include "vulkan_provider.hpp"

#include <corehost/static_core.hpp>
#include <ps5rt/app.hpp>
#include <ps5rt/log.hpp>

#include <libretro.h>
#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>

extern "C" {
unsigned retro_api_version(void);
void retro_set_environment(retro_environment_t);
void retro_set_video_refresh(retro_video_refresh_t);
void retro_set_audio_sample(retro_audio_sample_t);
void retro_set_audio_sample_batch(retro_audio_sample_batch_t);
void retro_set_input_poll(retro_input_poll_t);
void retro_set_input_state(retro_input_state_t);
void retro_init(void);
void retro_deinit(void);
void retro_get_system_info(struct retro_system_info*);
void retro_set_controller_port_device(unsigned, unsigned);
bool retro_load_game(const struct retro_game_info*);
void retro_unload_game(void);
void retro_get_system_av_info(struct retro_system_av_info*);
void retro_run(void);
void retro_reset(void);
std::size_t retro_serialize_size(void);
bool retro_serialize(void*, std::size_t);
bool retro_unserialize(const void*, std::size_t);
void* retro_get_memory_data(unsigned);
std::size_t retro_get_memory_size(unsigned);
}

namespace {

corehost::lr::StaticApi static_api() noexcept {
  return {
      reinterpret_cast<void(*)(corehost::lr::Environment)>(
          retro_set_environment),
      reinterpret_cast<void(*)(corehost::lr::VideoRefresh)>(
          retro_set_video_refresh),
      reinterpret_cast<void(*)(corehost::lr::AudioSample)>(
          retro_set_audio_sample),
      reinterpret_cast<void(*)(corehost::lr::AudioBatch)>(
          retro_set_audio_sample_batch),
      reinterpret_cast<void(*)(corehost::lr::InputPoll)>(
          retro_set_input_poll),
      reinterpret_cast<void(*)(corehost::lr::InputState)>(
          retro_set_input_state),
      retro_init,
      retro_deinit,
      reinterpret_cast<void(*)(corehost::lr::SystemInfo*)>(
          retro_get_system_info),
      retro_set_controller_port_device,
      reinterpret_cast<bool(*)(const corehost::lr::GameInfo*)>(
          retro_load_game),
      retro_unload_game,
      reinterpret_cast<void(*)(corehost::lr::SystemAvInfo*)>(
          retro_get_system_av_info),
      retro_run,
      retro_reset,
      retro_serialize_size,
      retro_serialize,
      retro_unserialize,
      retro_get_memory_data,
      retro_get_memory_size,
  };
}

std::string boot_content(int argc, char** argv) {
  if (argc > 1 && argv && argv[1] && argv[1][0])
    return argv[1];

  std::ifstream input("/data/NATIVE-EMUS-PS5/ps1/boot.txt");
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.front() != '#')
      return line;
  }
  return {};
}

int fail(std::string_view message, int code) {
  ps5rt::log(ps5rt::LogLevel::error, "ps1", message);
  return code;
}

} // namespace

int main(int argc, char** argv) {
  if (retro_api_version() != RETRO_API_VERSION)
    return fail("unexpected libretro ABI version", 2);

  ps5rt::AppInfo app{};
  const ps5rt::AppConfig app_config{
      "NATIVE-EMUS-PS5 · PlayStation",
      "/data/NATIVE-EMUS-PS5/ps1",
      true,
      false,
      true,
  };
  if (!ps5rt::initialize_app(app_config, app))
    return fail("PS5 application services failed to initialize", 3);

  native_emus::ps1::ContentLayout layout;
  native_emus::ps1::PreparedContent prepared;
  std::string error;
  const std::string content = boot_content(argc, argv);
  if (!native_emus::ps1::prepare_content(
          content, layout, prepared, error)) {
    ps5rt::shutdown_app();
    return fail(error, 4);
  }

  native_emus::ps1::VulkanEnvironment vulkan_environment;
  native_emus::ps1::VulkanPresenter presenter(vkGetInstanceProcAddr);
  native_emus::ps1::RuntimeIo runtime_io;

  corehost::StaticCore core(
      "Beetle PSX HW",
      static_api(),
      {layout.system_dir.string(), layout.save_dir.string()},
      runtime_io.make_hooks(
          vulkan_environment,
          [&presenter](std::uint32_t width, std::uint32_t height) {
            return presenter.present(width, height);
          }));

  // Standalone PS5 policy: hardware Vulkan + native recompiler. We never
  // silently downgrade the final PS5 target to the Beetle interpreter.
  core.set_option("beetle_psx_hw_renderer", "hardware_vk");
  core.set_option("beetle_psx_hw_cpu_dynarec", "execute");

  if (!core.initialize(error)) {
    ps5rt::shutdown_app();
    return fail(error, 5);
  }

  const auto sample_rate = core.av_info().timing.sample_rate;
  const auto rounded_rate = sample_rate >= 8000.0 && sample_rate <= 192000.0
      ? static_cast<std::uint32_t>(sample_rate + 0.5)
      : 44100u;
  if (!runtime_io.initialize(rounded_rate)) {
    ps5rt::shutdown_app();
    return fail("native PS5 audio/input initialization failed", 6);
  }

  if (!core.load_path(prepared.core_path, error)) {
    ps5rt::shutdown_app();
    return fail(error, 7);
  }

  native_emus::ps1::SaveRamStore save_ram(prepared.save_ram_path);
  if (!save_ram.load(core, error)) {
    core.unload();
    ps5rt::shutdown_app();
    return fail(error, 8);
  }

  native_emus::ps1::VulkanProvider provider(
      vulkan_environment, presenter.hooks());
  if (!provider.initialize(vkGetInstanceProcAddr)) {
    core.unload();
    ps5rt::shutdown_app();
    return fail("native PS5 Vulkan/RADV context initialization failed", 9);
  }

  constexpr std::uint64_t save_interval_frames = 60u * 30u;
  std::uint64_t frames = 0;

  while (!runtime_io.quit_requested()) {
    core.run_frame();
    ++frames;

    if ((frames % save_interval_frames) == 0) {
      std::string save_error;
      if (!save_ram.save(core, save_error))
        ps5rt::log(ps5rt::LogLevel::warning, "ps1", save_error);
    }
  }

  if (!save_ram.save(core, error))
    ps5rt::log(ps5rt::LogLevel::warning, "ps1", error);

  // Beetle closes its renderer while its VkDevice is still valid.
  core.unload();
  provider.shutdown();
  runtime_io.shutdown();
  core.shutdown();
  ps5rt::shutdown_app();
  return 0;
}
