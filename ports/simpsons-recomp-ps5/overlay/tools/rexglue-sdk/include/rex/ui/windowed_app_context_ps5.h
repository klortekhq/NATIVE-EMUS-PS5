#pragma once

#include <rex/ui/windowed_app_context.h>

namespace rex::ui {

class PS5WindowedAppContext final : public WindowedAppContext {
 public:
  PS5WindowedAppContext() = default;
  ~PS5WindowedAppContext() override = default;

  void NotifyUILoopOfPendingFunctions() override;
  void PlatformQuitFromUIThread() override;

  // PS5 has no desktop message queue for a fullscreen homebrew title. The
  // common WindowedAppContext queue is pumped here at a short interval.
  void RunMainLoop();
};

}  // namespace rex::ui
