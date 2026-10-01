#!/usr/bin/env python3
"""Deterministically adapt TheSimpsonsGameRecomp/ReXGlue for native PS5 bring-up.

This script intentionally uses exact replacements. Upstream drift must fail loudly
rather than silently generating a half-patched tree.
"""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

PORT_REL = Path("ports/simpsons-recomp-ps5")
OVERLAY_REL = PORT_REL / "overlay"


def replace_exact(path: Path, old: str, new: str, check: bool) -> bool:
    text = path.read_text(encoding="utf-8")
    if new in text:
        return False
    if old not in text:
        raise RuntimeError(f"expected block not found in {path}")
    if not check:
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
    return True


def copy_overlay(repo_root: Path, source_root: Path, check: bool) -> int:
    overlay = repo_root / OVERLAY_REL
    changed = 0
    for src in overlay.rglob("*"):
        if not src.is_file():
            continue
        rel = src.relative_to(overlay)
        dst = source_root / rel
        if dst.exists() and dst.read_bytes() == src.read_bytes():
            continue
        changed += 1
        if not check:
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
    return changed


def patch_sdk_top(source_root: Path, check: bool) -> int:
    p = source_root / "tools/rexglue-sdk/CMakeLists.txt"
    changed = 0
    changed += replace_exact(
        p,
        """# Platform detection (supports AMD64 and ARM64)
if(WIN32)
""",
        """# Platform detection (supports AMD64 and ARM64)
if(REXGLUE_PS5)
    set(REX_PLATFORM "ps5-amd64")
    add_compile_definitions(REX_PLATFORM_PS5=1)
    if(NOT TARGET ps5rt::ps5)
        if(NOT NATIVE_EMUS_ROOT)
            message(FATAL_ERROR "REXGLUE_PS5 requires NATIVE_EMUS_ROOT")
        endif()
        add_subdirectory("${NATIVE_EMUS_ROOT}/runtime" "${CMAKE_BINARY_DIR}/ps5rt")
    endif()
elseif(WIN32)
""",
        check,
    )
    return changed


def patch_ui(source_root: Path, check: bool) -> int:
    p = source_root / "tools/rexglue-sdk/src/ui/CMakeLists.txt"
    changed = 0
    changed += replace_exact(
        p,
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
        check,
    )
    changed += replace_exact(
        p,
        """# Platform-specific dependencies
if(WIN32)
    target_link_libraries(rexui PUBLIC
        dwmapi
        Shcore
    )
else()
""",
        """# Platform-specific dependencies
if(REXGLUE_PS5)
    target_link_libraries(rexui PUBLIC ps5rt::ps5)
elseif(WIN32)
    target_link_libraries(rexui PUBLIC
        dwmapi
        Shcore
    )
else()
""",
        check,
    )
    return changed


def patch_audio(source_root: Path, check: bool) -> int:
    p = source_root / "tools/rexglue-sdk/src/audio/CMakeLists.txt"
    old = """add_library(rexaudio OBJECT
    
    audio_driver.cpp
    audio_system.cpp
    xma_context.cpp
    xma_decoder.cpp
    xma_register_file.cpp
    # NOP backend (always available)
    nop/nop_audio_system.cpp
    # SDL backend
    sdl/sdl_audio_system.cpp
    sdl/sdl_audio_driver.cpp
)
add_library(rex::audio ALIAS rexaudio)

target_include_directories(rexaudio PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(rexaudio
    PUBLIC rexcore SDL3::SDL3
    PRIVATE libavcodec libavutil
)
"""
    new = """set(REXAUDIO_SOURCES
    audio_driver.cpp
    audio_system.cpp
    xma_context.cpp
    xma_decoder.cpp
    xma_register_file.cpp
    nop/nop_audio_system.cpp
)

if(REXGLUE_PS5)
    list(APPEND REXAUDIO_SOURCES
        ps5/ps5_audio_system.cpp
        ps5/ps5_audio_driver.cpp
    )
else()
    list(APPEND REXAUDIO_SOURCES
        sdl/sdl_audio_system.cpp
        sdl/sdl_audio_driver.cpp
    )
endif()

add_library(rexaudio OBJECT ${REXAUDIO_SOURCES})
add_library(rex::audio ALIAS rexaudio)

target_include_directories(rexaudio PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(rexaudio PUBLIC rexcore PRIVATE libavcodec libavutil)
if(REXGLUE_PS5)
    target_link_libraries(rexaudio PUBLIC ps5rt::ps5)
else()
    target_link_libraries(rexaudio PUBLIC SDL3::SDL3)
endif()
"""
    return int(replace_exact(p, old, new, check))


def patch_input(source_root: Path, check: bool) -> int:
    p = source_root / "tools/rexglue-sdk/src/input/CMakeLists.txt"
    changed = 0
    changed += replace_exact(
        p,
        """if(WIN32)
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp xinput/xinput_input_driver.cpp)
else()
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp)
endif()

target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)
""",
        """if(REXGLUE_PS5)
    target_sources(rexinput PRIVATE ps5/ps5_input_driver.cpp)
    target_link_libraries(rexinput PUBLIC rexcore rexui ps5rt::ps5)
elseif(WIN32)
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp xinput/xinput_input_driver.cpp)
    target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)
else()
    target_sources(rexinput PRIVATE sdl/sdl_input_driver.cpp)
    target_link_libraries(rexinput PUBLIC rexcore rexui SDL3::SDL3)
endif()
""",
        check,
    )
    return changed


def patch_helpers(source_root: Path, check: bool) -> int:
    p = source_root / "tools/rexglue-sdk/cmake/rexglue_helpers.cmake"
    changed = 0
    changed += replace_exact(
        p,
        """    if(UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
""",
        """    if(REXGLUE_PS5)
        target_link_libraries(${target_name} PRIVATE ps5rt::ps5)
        target_link_options(${target_name} PRIVATE -Wl,--no-relax)
        target_compile_options(${target_name} PRIVATE -mcmodel=large)
    elseif(UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
""",
        check,
    )
    changed += replace_exact(
        p,
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
        check,
    )
    return changed


def patch_thirdparty(source_root: Path, check: bool) -> int:
    p = source_root / "tools/rexglue-sdk/thirdparty/CMakeLists.txt"
    changed = 0
    changed += replace_exact(
        p,
        """    imgui
    sdl3
)
""",
        """    imgui
)
if(NOT REXGLUE_PS5)
    list(APPEND REQUIRED_SUBMODULES sdl3)
endif()
""",
        check,
    )
    changed += replace_exact(
        p,
        """#=============================================================================
# SDL3 - Cross-platform media layer
#=============================================================================
set(SDL_INSTALL ON CACHE BOOL "" FORCE)
""",
        """#=============================================================================
# SDL3 - Cross-platform media layer
#=============================================================================
if(NOT REXGLUE_PS5)
set(SDL_INSTALL ON CACHE BOOL "" FORCE)
""",
        check,
    )
    changed += replace_exact(
        p,
        """add_subdirectory(sdl3)

#=============================================================================
# DXC API headers
""",
        """add_subdirectory(sdl3)
endif()

#=============================================================================
# DXC API headers
""",
        check,
    )
    # Reuse the x86_64 Linux FFmpeg config as the first bring-up baseline.
    # It is build-time codec configuration, not a Linux UI dependency.
    changed += replace_exact(
        p,
        """if(WIN32)
    set(_ff_os windows)
elseif(APPLE)
    set(_ff_os macos)
else()
    set(_ff_os linux)
endif()
""",
        """if(WIN32)
    set(_ff_os windows)
elseif(APPLE)
    set(_ff_os macos)
elseif(REXGLUE_PS5)
    set(_ff_os linux)
else()
    set(_ff_os linux)
endif()
""",
        check,
    )
    return changed


def patch_game(source_root: Path, check: bool) -> int:
    p = source_root / "simpsons/CMakeLists.txt"
    return int(replace_exact(
        p,
        """if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
""",
        """if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64"
   AND NOT REXGLUE_PS5)
""",
        check,
    ))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path, help="TheSimpsonsGameRecomp checkout")
    ap.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    ap.add_argument("--check", action="store_true")
    ns = ap.parse_args()

    source = ns.source.resolve()
    repo_root = ns.repo_root.resolve()
    if not (source / "simpsons/CMakeLists.txt").is_file():
        raise SystemExit("not a TheSimpsonsGameRecomp checkout")

    changes = copy_overlay(repo_root, source, ns.check)
    for fn in (
        patch_sdk_top,
        patch_ui,
        patch_audio,
        patch_input,
        patch_helpers,
        patch_thirdparty,
        patch_game,
    ):
        changes += fn(source, ns.check)

    mode = "would change" if ns.check else "changed"
    print(f"PS5 transform: {mode} {changes} item(s)")
    return 1 if ns.check and changes else 0


if __name__ == "__main__":
    raise SystemExit(main())
