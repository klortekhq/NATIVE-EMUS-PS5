#include <chrono>
#include <thread>

#include <rex/assert.h>
#include <rex/ui/windowed_app_context_ps5.h>

namespace rex::ui {

void PS5WindowedAppContext::NotifyUILoopOfPendingFunctions() {
  // RunMainLoop polls the common queue. No platform wake object is required.
}

void PS5WindowedAppContext::PlatformQuitFromUIThread() {
  // QuitFromUIThread has already marked the base context as quit before this
  // callback is reached. There is no separate OS event loop to terminate.
}

void PS5WindowedAppContext::RunMainLoop() {
  assert_true(IsInUIThread());
  while (!HasQuitFromUIThread()) {
    ExecutePendingFunctionsFromUIThread();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  // Drain work that was accepted before the quit transition.
  ExecutePendingFunctionsFromUIThread();
}

}  // namespace rex::ui
