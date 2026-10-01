#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ps5rt/app.hpp>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_ps5.h>

#ifndef REX_PS5_GAME_DATA_ROOT
#define REX_PS5_GAME_DATA_ROOT "/data/homebrew/TheSimpsonsGame/game"
#endif
#ifndef REX_PS5_USER_DATA_ROOT
#define REX_PS5_USER_DATA_ROOT "/data/homebrew/TheSimpsonsGame/user"
#endif
#ifndef REX_PS5_CACHE_ROOT
#define REX_PS5_CACHE_ROOT "/data/homebrew/TheSimpsonsGame/cache"
#endif

extern "C" int main() {
  ps5rt::AppInfo app_info{};
  ps5rt::AppConfig app_config{};
  app_config.app_name = "The Simpsons Game Recompiled";
  app_config.data_directory = REX_PS5_USER_DATA_ROOT;
  app_config.initialize_user_service = true;
  app_config.initialize_network = false;
  app_config.enable_crash_log = true;
  if (!ps5rt::initialize_app(app_config, app_info)) {
    return EXIT_FAILURE;
  }

  // Feed normal ReXGlue cvars rather than special-casing game paths deeper in
  // the runtime. This preserves the same path resolution code as upstream.
  std::string arg0 = "simpsons";
  std::string game_arg = std::string("--game_data_root=") + REX_PS5_GAME_DATA_ROOT;
  std::string user_arg = std::string("--user_data_root=") + REX_PS5_USER_DATA_ROOT;
  std::string cache_arg = std::string("--cache_path=") + REX_PS5_CACHE_ROOT;
  std::vector<char*> argv = {arg0.data(), game_arg.data(), user_arg.data(), cache_arg.data()};
  auto remaining = rex::cvar::Init(static_cast<int>(argv.size()), argv.data());
  rex::InitLoggingEarly();

  int result = EXIT_FAILURE;
  {
    rex::ui::PS5WindowedAppContext app_context;
    std::unique_ptr<rex::ui::WindowedApp> app =
        rex::ui::GetWindowedAppCreator()(app_context);

    const auto& option_names = app->GetPositionalOptions();
    std::map<std::string, std::string> parsed;
    const size_t count = std::min(remaining.size(), option_names.size());
    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }
    app->SetParsedArguments(std::move(parsed));

    if (app->OnInitialize()) {
      app_context.RunMainLoop();
      result = EXIT_SUCCESS;
    }
    app->InvokeOnDestroy();
  }

  rex::ShutdownLogging();
  ps5rt::shutdown_app();
  return result;
}
