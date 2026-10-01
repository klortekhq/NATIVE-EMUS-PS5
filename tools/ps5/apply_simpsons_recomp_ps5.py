#!/usr/bin/env python3
"""Deterministically retarget TheSimpsonsGameRecomp/ReXGlue to native PS5.

The transform is deliberately strict:
- one exact upstream revision;
- clean checkout;
- exact source replacements;
- PS5 platform identity separated from Linux;
- PS5 overlay copied from this repository.

It is a bring-up transform, not a claim that the final title already links.
"""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess

EXPECTED = "15feabc95291a7a50851a9a184c5bd78d2dd1f4d"
PLATFORM = pathlib.Path("tools/rexglue-sdk/include/rex/platform.h")
SDK_CMAKE = pathlib.Path("tools/rexglue-sdk/CMakeLists.txt")
UI_CMAKE = pathlib.Path("tools/rexglue-sdk/src/ui/CMakeLists.txt")
HELPERS = pathlib.Path("tools/rexglue-sdk/cmake/rexglue_helpers.cmake")
SIMPSONS_CMAKE = pathlib.Path("simpsons/CMakeLists.txt")
SURFACE = pathlib.Path("tools/rexglue-sdk/include/rex/ui/surface.h")
AUDIO_CMAKE = pathlib.Path("tools/rexglue-sdk/src/audio/CMakeLists.txt")
INPUT_CMAKE = pathlib.Path("tools/rexglue-sdk/src/input/CMakeLists.txt")
INPUT_SYSTEM = pathlib.Path("tools/rexglue-sdk/src/input/input_system.cpp")
REX_APP = pathlib.Path("tools/rexglue-sdk/src/ui/rex_app.cpp")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old, new, 1)


def transform_platform(text: str) -> str:
    text = replace_once(
        text,
        """#elif defined(__ANDROID__)
#define REX_PLATFORM_ANDROID 1
#define REX_PLATFORM_LINUX 1
#elif defined(__gnu_linux__)
""",
        """#elif defined(__ANDROID__)
#define REX_PLATFORM_ANDROID 1
#define REX_PLATFORM_LINUX 1
#elif defined(__PROSPERO__)
#define REX_PLATFORM_PS5 1
#elif defined(__gnu_linux__)
""",
        "PS5 platform identity",
    )
    text = replace_once(
        text,
        """#ifndef REX_PLATFORM_LINUX
#define REX_PLATFORM_LINUX 0
#endif
""",
        """#ifndef REX_PLATFORM_LINUX
#define REX_PLATFORM_LINUX 0
#endif
#ifndef REX_PLATFORM_PS5
#define REX_PLATFORM_PS5 0
#endif
""",
        "PS5 platform default",
    )
    return text


def transform_sdk_cmake(text: str) -> str:
    return replace_once(
        text,
        """# Platform detection (supports AMD64 and ARM64)
if(WIN32)
""",
        """# Platform detection (supports AMD64 and ARM64)
# PS5 uses the public Prospero compiler/sysroot. Keep it distinct from Linux
# even though CMake's cross-toolchain identifies the target as FreeBSD.
if(REXGLUE_PS5)
    set(REX_PLATFORM "ps5-amd64")
    add_compile_definitions(REX_PLATFORM_PS5=1)
elseif(WIN32)
""",
        "ReXGlue PS5 CMake platform",
    )


def transform_ui_cmake(text: str) -> str:
    text = replace_once(
        text,
        """# Platform-specific sources
if(WIN32)
    set(REXUI_PLATFORM_SOURCES
        surface_win.cpp
        window_win.cpp
        windowed_app_context_win.cpp
    )
else()
    set(REXUI_PLATFORM_SOURCES
        surface_gnulinux.cpp
        window_gtk.cpp
        windowed_app_context_gtk.cpp
    )
endif()
""",
        """# Platform-specific sources
if(REXGLUE_PS5)
    set(REXUI_PLATFORM_SOURCES
        surface_ps5.cpp
        window_ps5.cpp
        windowed_app_context_ps5.cpp
    )
elseif(WIN32)
    set(REXUI_PLATFORM_SOURCES
        surface_win.cpp
        window_win.cpp
        windowed_app_context_win.cpp
    )
else()
    set(REXUI_PLATFORM_SOURCES
        surface_gnulinux.cpp
        window_gtk.cpp
        windowed_app_context_gtk.cpp
    )
endif()
""",
        "PS5 UI sources",
    )
    text = replace_once(
        text,
        """# Platform-specific dependencies
if(WIN32)
    target_link_libraries(rexui PUBLIC
        dwmapi
        Shcore
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(X11_XCB REQUIRED x11-xcb)

    target_include_directories(rexui PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${X11_XCB_INCLUDE_DIRS}
    )
    target_link_libraries(rexui PUBLIC
        ${GTK3_LIBRARIES}
        ${X11_XCB_LIBRARIES}
    )
endif()
""",
        """# Platform-specific dependencies
if(REXGLUE_PS5)
    # Native fullscreen window/event glue has no GTK/X11 dependency.
elseif(WIN32)
    target_link_libraries(rexui PUBLIC
        dwmapi
        Shcore
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(X11_XCB REQUIRED x11-xcb)

    target_include_directories(rexui PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${X11_XCB_INCLUDE_DIRS}
    )
    target_link_libraries(rexui PUBLIC
        ${GTK3_LIBRARIES}
        ${X11_XCB_LIBRARIES}
    )
endif()
""",
        "PS5 UI dependencies",
    )
    return text


def transform_helpers(text: str) -> str:
    text = replace_once(
        text,
        """    if(UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        target_include_directories(${target_name} PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(${target_name} PRIVATE ${GTK3_LIBRARIES})
        # Large executable support
""",
        """    if(UNIX AND NOT APPLE AND NOT REXGLUE_PS5)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        target_include_directories(${target_name} PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(${target_name} PRIVATE ${GTK3_LIBRARIES})
        # Large executable support
""",
        "avoid GTK on PS5",
    )
    text = replace_once(
        text,
        """    if(WIN32)
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_win.cpp)
    else()
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_posix.cpp)
    endif()
""",
        """    if(REXGLUE_PS5)
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_ps5.cpp)
    elseif(WIN32)
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_win.cpp)
    else()
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_posix.cpp)
    endif()
""",
        "PS5 application entry point",
    )
    return text


def transform_simpsons_cmake(text: str) -> str:
    text = replace_once(
        text,
        """include(generated/rexglue.cmake)
""",
        """if(REXGLUE_PS5)
    if(NOT NATIVE_EMUS_ROOT)
        message(FATAL_ERROR "NATIVE_EMUS_ROOT is required for the PS5 port")
    endif()
    set(PS5RT_BUILD_PS5_BACKEND ON CACHE BOOL "" FORCE)
    add_subdirectory("${NATIVE_EMUS_ROOT}/runtime" "${CMAKE_BINARY_DIR}/ps5rt")
endif()

include(generated/rexglue.cmake)
""",
        "ps5rt integration",
    )
    text = replace_once(
        text,
        """rexglue_setup_target(simpsons)
""",
        """rexglue_setup_target(simpsons)

if(REXGLUE_PS5)
    target_link_libraries(simpsons PRIVATE ps5rt::ps5)
    target_compile_definitions(simpsons PRIVATE REX_PLATFORM_PS5=1)
endif()
""",
        "PS5 target link",
    )
    return text



def transform_surface(text: str) -> str:
    text = replace_once(
        text,
        """    // Windows.
    kTypeIndex_Win32Hwnd,
""",
        """    // Windows.
    kTypeIndex_Win32Hwnd,
    // PlayStation 5 fullscreen display.
    kTypeIndex_PS5Display,
""",
        "PS5 surface enum",
    )
    text = replace_once(
        text,
        """    kTypeFlag_Win32Hwnd = TypeFlags(1) << kTypeIndex_Win32Hwnd,
""",
        """    kTypeFlag_Win32Hwnd = TypeFlags(1) << kTypeIndex_Win32Hwnd,
    kTypeFlag_PS5Display = TypeFlags(1) << kTypeIndex_PS5Display,
""",
        "PS5 surface flag",
    )
    return text


def transform_audio_cmake(text: str) -> str:
    text = replace_once(
        text,
        """    # SDL backend
    sdl/sdl_audio_system.cpp
    sdl/sdl_audio_driver.cpp
)
""",
        """    # Host backend selected below.
)

if(REXGLUE_PS5)
    target_sources(rexaudio PRIVATE
        ps5/ps5_audio_system.cpp
        ps5/ps5_audio_driver.cpp
    )
else()
    target_sources(rexaudio PRIVATE
        sdl/sdl_audio_system.cpp
        sdl/sdl_audio_driver.cpp
    )
endif()
""",
        "PS5 audio sources",
    )
    text = replace_once(
        text,
        """target_link_libraries(rexaudio
    PUBLIC rexcore SDL3::SDL3
    PRIVATE libavcodec libavutil
)
""",
        """target_link_libraries(rexaudio
    PUBLIC rexcore
    PRIVATE libavcodec libavutil
)
if(REXGLUE_PS5)
    target_link_libraries(rexaudio PRIVATE ps5rt::ps5)
else()
    target_link_libraries(rexaudio PUBLIC SDL3::SDL3)
endif()
""",
        "PS5 audio dependencies",
    )
    return text


def transform_input_cmake(text: str) -> str:
    text = replace_once(
        text,
        """add_library(rexinput OBJECT
    input_system.cpp
    mnk/mnk_input_driver.cpp
    nop/nop_input_driver.cpp
)
""",
        """add_library(rexinput OBJECT
    input_system.cpp
    nop/nop_input_driver.cpp
)
if(REXGLUE_PS5)
    target_sources(rexinput PRIVATE ps5/ps5_input_driver.cpp)
else()
    target_sources(rexinput PRIVATE mnk/mnk_input_driver.cpp)
endif()
""",
        "PS5 input sources",
    )
    text = replace_once(
        text,
        """if(WIN32)
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp xinput/xinput_input_driver.cpp)
else()
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp)
endif()

target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)
""",
        """if(REXGLUE_PS5)
    target_link_libraries(rexinput PUBLIC rexcore rexui PRIVATE ps5rt::ps5)
elseif(WIN32)
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp xinput/xinput_input_driver.cpp)
    target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)
else()
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp)
    target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)
endif()
""",
        "PS5 input dependencies",
    )
    return text


def transform_input_system(text: str) -> str:
    text = replace_once(
        text,
        """#include <rex/input/nop/nop_input_driver.h>
#include <rex/input/sdl/sdl_input_driver.h>
#include <rex/input/xinput/xinput_input_driver.h>
#include <rex/logging.h>

REXCVAR_DEFINE_STRING(input_backend, "sdl", "Input", "Input backend: sdl, xinput")
    .allowed({"sdl", "xinput"});
""",
        """#include <rex/input/nop/nop_input_driver.h>
#include <rex/platform.h>
#if REX_PLATFORM_PS5
#include <rex/input/ps5/ps5_input_driver.h>
#else
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/input/sdl/sdl_input_driver.h>
#include <rex/input/xinput/xinput_input_driver.h>
#endif
#include <rex/logging.h>

#if REX_PLATFORM_PS5
REXCVAR_DEFINE_STRING(input_backend, "ps5", "Input", "Input backend: ps5")
    .allowed({"ps5"});
#else
REXCVAR_DEFINE_STRING(input_backend, "sdl", "Input", "Input backend: sdl, xinput")
    .allowed({"sdl", "xinput"});
#endif
""",
        "PS5 input includes and cvar",
    )
    text = replace_once(
        text,
        """  if (!tool_mode) {
#if REX_PLATFORM_WIN32
    if (REXCVAR_GET(input_backend) == "xinput") {
      auto xinput_driver = std::make_unique<xinput::XinputInputDriver>(nullptr, 0);
      if (xinput_driver->Setup() == X_STATUS_SUCCESS) {
        input->AddDriver(std::move(xinput_driver));
      }
    }
#endif

    if (REXCVAR_GET(input_backend) == "sdl") {
      auto sdl_driver = std::make_unique<sdl::SDLInputDriver>(nullptr, 0);
      if (sdl_driver->Setup() == X_STATUS_SUCCESS) {
        input->AddDriver(std::move(sdl_driver));
      }
    }

    // MnK driver (keyboard/mouse -> controller emulation)
    auto mnk_driver = std::make_unique<mnk::MnkInputDriver>(nullptr, 0);
    if (mnk_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(mnk_driver));
    }
  }
""",
        """  if (!tool_mode) {
#if REX_PLATFORM_PS5
    auto ps5_driver = std::make_unique<ps5::PS5InputDriver>(nullptr, 0);
    if (ps5_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(ps5_driver));
    }
#else
#if REX_PLATFORM_WIN32
    if (REXCVAR_GET(input_backend) == "xinput") {
      auto xinput_driver = std::make_unique<xinput::XinputInputDriver>(nullptr, 0);
      if (xinput_driver->Setup() == X_STATUS_SUCCESS) {
        input->AddDriver(std::move(xinput_driver));
      }
    }
#endif

    if (REXCVAR_GET(input_backend) == "sdl") {
      auto sdl_driver = std::make_unique<sdl::SDLInputDriver>(nullptr, 0);
      if (sdl_driver->Setup() == X_STATUS_SUCCESS) {
        input->AddDriver(std::move(sdl_driver));
      }
    }

    auto mnk_driver = std::make_unique<mnk::MnkInputDriver>(nullptr, 0);
    if (mnk_driver->Setup() == X_STATUS_SUCCESS) {
      input->AddDriver(std::move(mnk_driver));
    }
#endif
  }
""",
        "PS5 input factory",
    )
    return text


def transform_rex_app(text: str) -> str:
    text = replace_once(
        text,
        """#include <rex/audio/audio_system.h>
#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/input/input_system.h>
""",
        """#include <rex/audio/audio_system.h>
#include <rex/platform.h>
#if REX_PLATFORM_PS5
#include <rex/audio/ps5/ps5_audio_system.h>
#else
#include <rex/audio/sdl/sdl_audio_system.h>
#endif
#include <rex/input/input_system.h>
""",
        "PS5 audio include",
    )
    text = replace_once(
        text,
        """  config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
  config_.input_factory = REX_INPUT_BACKEND(rex::input::CreateDefaultInputSystem);
""",
        """#if REX_PLATFORM_PS5
  config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::ps5::PS5AudioSystem);
#else
  config_.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
#endif
  config_.input_factory = REX_INPUT_BACKEND(rex::input::CreateDefaultInputSystem);
""",
        "PS5 audio factory",
    )
    return text

def copy_overlay(source_root: pathlib.Path, repo_root: pathlib.Path, check: bool) -> None:
    overlay = repo_root / "ports/simpsons-recomp-ps5/overlay"
    if not overlay.is_dir():
        raise RuntimeError(f"missing PS5 overlay: {overlay}")
    for src in sorted(p for p in overlay.rglob("*") if p.is_file()):
        rel = src.relative_to(overlay)
        dst = source_root / rel
        if check:
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=pathlib.Path)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    root = args.source.resolve()
    repo_root = pathlib.Path(__file__).resolve().parents[2]

    head = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong Simpsons revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("TheSimpsonsGameRecomp checkout has local modifications")

    try:
        transformed = {
            PLATFORM: transform_platform((root / PLATFORM).read_text()),
            SDK_CMAKE: transform_sdk_cmake((root / SDK_CMAKE).read_text()),
            UI_CMAKE: transform_ui_cmake((root / UI_CMAKE).read_text()),
            HELPERS: transform_helpers((root / HELPERS).read_text()),
            SIMPSONS_CMAKE: transform_simpsons_cmake((root / SIMPSONS_CMAKE).read_text()),
            SURFACE: transform_surface((root / SURFACE).read_text()),
            AUDIO_CMAKE: transform_audio_cmake((root / AUDIO_CMAKE).read_text()),
            INPUT_CMAKE: transform_input_cmake((root / INPUT_CMAKE).read_text()),
            INPUT_SYSTEM: transform_input_system((root / INPUT_SYSTEM).read_text()),
            REX_APP: transform_rex_app((root / REX_APP).read_text()),
        }
        copy_overlay(root, repo_root, args.check)
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        checks = {
            "platform": "#define REX_PLATFORM_PS5 1" in transformed[PLATFORM],
            "cmake": 'set(REX_PLATFORM "ps5-amd64")' in transformed[SDK_CMAKE],
            "ui": "surface_ps5.cpp" in transformed[UI_CMAKE],
            "main": "windowed_app_main_ps5.cpp" in transformed[HELPERS],
            "ps5rt": "ps5rt::ps5" in transformed[SIMPSONS_CMAKE],
            "surface": "kTypeFlag_PS5Display" in transformed[SURFACE],
            "audio": "ps5_audio_driver.cpp" in transformed[AUDIO_CMAKE],
            "input": "ps5_input_driver.cpp" in transformed[INPUT_CMAKE],
            "input_factory": "PS5InputDriver" in transformed[INPUT_SYSTEM],
            "audio_factory": "PS5AudioSystem" in transformed[REX_APP],
        }
        failed = [name for name, good in checks.items() if not good]
        if failed:
            raise SystemExit("PS5 transform validation failed: " + ", ".join(failed))
        print(f"TheSimpsonsGameRecomp {EXPECTED}: PS5 transform matched")
        return 0

    for path, content in transformed.items():
        (root / path).write_text(content)

    copy_overlay(root, repo_root, False)
    (root / ".native-emus-ps5-simpsons").write_text(EXPECTED + "\n")
    print("The Simpsons Game Recomp retargeted for native PS5 bring-up")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
