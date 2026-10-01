#pragma once

#include <cstdint>

#include <rex/ui/surface.h>

namespace rex::ui {

// PS5 homebrew is a fixed fullscreen presentation target. The Vulkan presenter
// owns the real VK_KHR_display object; this class only carries the logical
// surface identity and the desired visible extent.
class PS5DisplaySurface final : public Surface {
 public:
  PS5DisplaySurface(uint32_t width, uint32_t height) : width_(width), height_(height) {}

  TypeIndex GetType() const override { return kTypeIndex_PS5Display; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override {
    width_out = width_;
    height_out = height_;
    return width_ != 0 && height_ != 0;
  }

 private:
  uint32_t width_;
  uint32_t height_;
};

}  // namespace rex::ui
