cmake_minimum_required(VERSION 3.20)

project(ps5rt LANGUAGES C CXX)

add_library(ps5rt INTERFACE)
add_library(ps5rt::ps5rt ALIAS ps5rt)

target_compile_features(ps5rt INTERFACE cxx_std_20)

target_include_directories(ps5rt
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# The concrete PS5 implementation will live behind this public contract.
# Emulator cores should depend on ps5rt::ps5rt rather than on another
# emulator/frontend.

option(PS5RT_BUILD_HEADER_TEST "Build the ps5rt public-header smoke test" OFF)

if(PS5RT_BUILD_HEADER_TEST)
  add_executable(ps5rt-header-test tests/header_compile.cpp)
  target_link_libraries(ps5rt-header-test PRIVATE ps5rt::ps5rt)

  add_executable(ps5rt-media-layout-test tests/media_layout_test.cpp)
  target_link_libraries(ps5rt-media-layout-test PRIVATE ps5rt::ps5rt)
  add_test(NAME ps5rt-media-layout COMMAND ps5rt-media-layout-test)
endif()

option(PS5RT_BUILD_PS5_BACKEND "Build the native PS5 backend implementation" OFF)

if(PS5RT_BUILD_PS5_BACKEND)
  add_library(ps5rt_ps5 STATIC
    src/ps5/app.cpp
    src/ps5/audio.cpp
    src/ps5/input.cpp
    src/ps5/input_hid.cpp
    src/ps5/io.cpp
    src/ps5/log.cpp
    src/ps5/memory.cpp
    src/ps5/vfs.cpp
    src/ps5/video.cpp
  )
  add_library(ps5rt::ps5 ALIAS ps5rt_ps5)
  target_link_libraries(ps5rt_ps5 PUBLIC ps5rt::ps5rt)
  target_compile_features(ps5rt_ps5 PUBLIC cxx_std_20)
  target_link_libraries(ps5rt_ps5
    PRIVATE
      SceAudioOut
      ScePad
      SceUserService
      SceVideoOut
      SceSystemService
  )
  option(PS5RT_BUILD_PS5_PROBES "Build native PS5 runtime probes" OFF)
  if(PS5RT_BUILD_PS5_PROBES)
    add_executable(ps5rt-jit-alias-probe probes/jit_alias_probe.cpp)
    target_link_libraries(ps5rt-jit-alias-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-jit-alias-probe PRIVATE cxx_std_20)
  endif()
endif()

option(PS5RT_BUILD_HOST_PLATFORM_TESTS "Build host tests for PS5 platform logic with mocked SCE calls" OFF)

if(PS5RT_BUILD_HOST_PLATFORM_TESTS)
  add_executable(ps5rt-input-host-test
    tests/input_ps5_host_test.cpp
    src/ps5/input.cpp
  )
  target_link_libraries(ps5rt-input-host-test PRIVATE ps5rt::ps5rt)
  target_compile_features(ps5rt-input-host-test PRIVATE cxx_std_20)
  add_test(NAME ps5rt-input-host COMMAND ps5rt-input-host-test)

  add_executable(ps5rt-memory-host-test
    tests/memory_ps5_host_test.cpp
    src/ps5/memory.cpp
  )
  target_link_libraries(ps5rt-memory-host-test PRIVATE ps5rt::ps5rt)
  target_compile_features(ps5rt-memory-host-test PRIVATE cxx_std_20)
  add_test(NAME ps5rt-memory-host COMMAND ps5rt-memory-host-test)

  add_executable(ps5rt-log-host-test
    tests/log_host_test.cpp
    src/ps5/log.cpp
  )
  target_link_libraries(ps5rt-log-host-test PRIVATE ps5rt::ps5rt)
  target_compile_features(ps5rt-log-host-test PRIVATE cxx_std_20)
  add_test(NAME ps5rt-log-host COMMAND ps5rt-log-host-test)
endif()
