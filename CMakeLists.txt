cmake_minimum_required(VERSION 3.20)

project(NATIVE_EMUS_PS5 LANGUAGES C CXX)

option(NATIVE_EMUS_BUILD_HEADER_TESTS
  "Build host-side compile smoke tests for the shared runtime headers"
  OFF)

set(PS5RT_BUILD_HEADER_TEST ${NATIVE_EMUS_BUILD_HEADER_TESTS})
add_subdirectory(runtime)

# Emulator cores are intentionally not added here until an upstream revision
# and license-compatible integration have been pinned for that workspace.
