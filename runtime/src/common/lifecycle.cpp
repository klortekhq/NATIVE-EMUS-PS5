#include <ps5rt/lifecycle.hpp>

namespace ps5rt {

ShutdownStack::~ShutdownStack() {
  run();
}

bool ShutdownStack::push(
    ShutdownCallback callback,
    void* context) noexcept {
  if (callback == nullptr ||
      size_ >= actions_.size()) {
    return false;
  }

  actions_[size_++] = {
      callback,
      context,
  };
  return true;
}

void ShutdownStack::run() noexcept {
  while (size_ != 0) {
    const Action action =
        actions_[--size_];
    actions_[size_] = {};

    if (action.callback != nullptr) {
      action.callback(action.context);
    }
  }
}

void ShutdownStack::clear() noexcept {
  while (size_ != 0) {
    actions_[--size_] = {};
  }
}

} // namespace ps5rt
