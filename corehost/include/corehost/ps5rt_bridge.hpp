#pragma once

#include <cstdint>

#include <corehost/static_core.hpp>
#include <ps5rt/input.hpp>
#include <ps5rt/video.hpp>

namespace corehost {

[[nodiscard]] InputState translate_ps5rt_input(
    const ps5rt::InputSnapshot& snapshot,
    unsigned port) noexcept;

[[nodiscard]] ps5rt::PixelFormat translate_pixel_format(
    lr::PixelFormat format) noexcept;

} // namespace corehost
