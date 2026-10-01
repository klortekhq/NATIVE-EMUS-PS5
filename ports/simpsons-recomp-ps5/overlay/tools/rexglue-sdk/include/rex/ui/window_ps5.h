#pragma once

#include <atomic>
#include <memory>

#include <rex/ui/window.h>

namespace rex::ui {

class PS5Window final : public Window {
 public:
  PS5Window(WindowedAppContext& app_context, std::string_view title,
            uint32_t desired_logical_width, uint32_t desired_logical_height);
  ~PS5Window() override;

  uint32_t GetMediumDpi() const override { return 96; }

 protected:
  bool OpenImpl() override;
  void RequestCloseImpl() override;
  void ApplyNewFullscreen() override;
  void FocusImpl() override;
  std::unique_ptr<Surface> CreateSurfaceImpl(Surface::TypeFlags allowed_types) override;
  void RequestPaintImpl() override;

 private:
  static constexpr uint32_t kDefaultWidth = 1920;
  static constexpr uint32_t kDefaultHeight = 1080;

  std::shared_ptr<std::atomic<bool>> alive_;
};

}  // namespace rex::ui
