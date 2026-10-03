#pragma once

#include <array>
#include <cstddef>

namespace ps5rt {

using ShutdownCallback = void (*)(void*) noexcept;

inline constexpr std::size_t max_shutdown_actions = 16;

class ShutdownStack {
public:
  ShutdownStack() = default;
  ShutdownStack(const ShutdownStack&) = delete;
  ShutdownStack& operator=(const ShutdownStack&) = delete;
  ShutdownStack(ShutdownStack&&) = delete;
  ShutdownStack& operator=(ShutdownStack&&) = delete;
  ~ShutdownStack();

  [[nodiscard]] bool push(
      ShutdownCallback callback,
      void* context = nullptr) noexcept;

  void run() noexcept;
  void clear() noexcept;

  [[nodiscard]] std::size_t size() const noexcept {
    return size_;
  }

  [[nodiscard]] bool empty() const noexcept {
    return size_ == 0;
  }

private:
  struct Action {
    ShutdownCallback callback{};
    void* context{};
  };

  std::array<Action, max_shutdown_actions> actions_{};
  std::size_t size_{};
};

} // namespace ps5rt
