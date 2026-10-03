#include <ps5rt/lifecycle.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

namespace {

struct Recorder {
  std::vector<int> calls{};
};

struct Entry {
  Recorder* recorder{};
  int value{};
};

void record(void* raw) noexcept {
  auto& entry = *static_cast<Entry*>(raw);
  entry.recorder->calls.push_back(entry.value);
}

void increment(void* raw) noexcept {
  auto& value = *static_cast<int*>(raw);
  ++value;
}

} // namespace

int main() {
  {
    Recorder recorder;
    Entry first{&recorder, 1};
    Entry second{&recorder, 2};
    Entry third{&recorder, 3};

    ps5rt::ShutdownStack stack;
    assert(stack.push(record, &first));
    assert(stack.push(record, &second));
    assert(stack.push(record, &third));
    assert(stack.size() == 3);

    stack.run();
    assert(stack.empty());
    assert((recorder.calls == std::vector<int>{3, 2, 1}));

    // Idempotent after the first drain.
    stack.run();
    assert(recorder.calls.size() == 3);
  }

  {
    int value = 0;
    {
      ps5rt::ShutdownStack stack;
      assert(stack.push(increment, &value));
    }
    assert(value == 1);
  }

  {
    int value = 0;
    ps5rt::ShutdownStack stack;
    assert(stack.push(increment, &value));
    stack.clear();
    stack.run();
    assert(value == 0);
  }

  {
    std::array<int, ps5rt::max_shutdown_actions> values{};
    ps5rt::ShutdownStack stack;
    for (auto& value : values) {
      assert(stack.push(increment, &value));
    }
    assert(!stack.push(increment, &values.front()));
    stack.run();
    for (const auto value : values) {
      assert(value == 1);
    }
  }

  {
    ps5rt::ShutdownStack stack;
    assert(!stack.push(nullptr));
  }

  return 0;
}
