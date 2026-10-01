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
