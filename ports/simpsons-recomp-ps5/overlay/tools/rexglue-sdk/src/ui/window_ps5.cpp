#include <memory>

#include <rex/ui/surface_ps5.h>
#include <rex/ui/window_ps5.h>

namespace rex::ui {

std::unique_ptr<Window> Window::Create(WindowedAppContext& app_context,
                                       const std::string_view title,
                                       uint32_t desired_logical_width,
                                       uint32_t desired_logical_height) {
  return std::make_unique<PS5Window>(app_context, title, desired_logical_width,
                                     desired_logical_height);
}

PS5Window::PS5Window(WindowedAppContext& app_context, std::string_view title,
                     uint32_t desired_logical_width, uint32_t desired_logical_height)
    : Window(app_context, title, desired_logical_width, desired_logical_height),
      alive_(std::make_shared<std::atomic<bool>>(true)) {}

PS5Window::~PS5Window() {
  alive_->store(false, std::memory_order_release);
  EnterDestructor();
}

bool PS5Window::OpenImpl() {
  WindowDestructionReceiver destruction_receiver(this);

  // A PS5 title is always a fullscreen display surface.
  OnDesiredFullscreenUpdate(true);
  OnDesiredLogicalSizeUpdate(kDefaultWidth, kDefaultHeight);
  OnActualSizeUpdate(kDefaultWidth, kDefaultHeight, destruction_receiver);
  if (destruction_receiver.IsWindowDestroyed()) {
    return true;
  }
  OnFocusUpdate(true, destruction_receiver);
  return true;
}

void PS5Window::RequestCloseImpl() {
  WindowDestructionReceiver destruction_receiver(this);
  OnBeforeClose(destruction_receiver);
  if (destruction_receiver.IsWindowDestroyed()) {
    return;
  }
  OnAfterClose();
}

void PS5Window::ApplyNewFullscreen() {
  // Fullscreen is the only supported presentation mode.
  OnDesiredFullscreenUpdate(true);
}

void PS5Window::FocusImpl() {
  WindowDestructionReceiver destruction_receiver(this);
  OnFocusUpdate(true, destruction_receiver);
}

std::unique_ptr<Surface> PS5Window::CreateSurfaceImpl(Surface::TypeFlags allowed_types) {
  if (!(allowed_types & Surface::kTypeFlag_PS5Display)) {
    return nullptr;
  }
  return std::make_unique<PS5DisplaySurface>(kDefaultWidth, kDefaultHeight);
}

void PS5Window::RequestPaintImpl() {
  auto alive = alive_;
  app_context().CallInUIThreadDeferred([this, alive]() {
    if (!alive->load(std::memory_order_acquire)) {
      return;
    }
    OnPaint(false);
  });
}

}  // namespace rex::ui
