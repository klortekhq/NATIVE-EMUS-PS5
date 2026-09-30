#include "../native/vulkan_presenter.hpp"

#include <cassert>

int main() {
  using native_emus::ps1::fit_present_rect;

  {
    const auto r = fit_present_rect(320, 240, {3840, 2160});
    assert(r.x0 == 480);
    assert(r.y0 == 0);
    assert(r.x1 == 3360);
    assert(r.y1 == 2160);
  }

  {
    const auto r = fit_present_rect(640, 480, {1920, 1080});
    assert(r.x0 == 240);
    assert(r.y0 == 0);
    assert(r.x1 == 1680);
    assert(r.y1 == 1080);
  }

  {
    const auto r = fit_present_rect(320, 240, {1280, 1024});
    assert(r.x0 == 0);
    assert(r.y0 == 32);
    assert(r.x1 == 1280);
    assert(r.y1 == 992);
  }

  {
    const auto r = fit_present_rect(0, 240, {1920, 1080});
    assert(r.x0 == 0 && r.y0 == 0 && r.x1 == 0 && r.y1 == 0);
  }

  return 0;
}
