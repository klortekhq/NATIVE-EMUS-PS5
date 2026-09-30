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
endif()

option(PS5RT_BUILD_PS5_BACKEND "Build the native PS5 backend implementation" OFF)

if(PS5RT_BUILD_PS5_BACKEND)
  add_library(ps5rt_ps5 STATIC
    src/ps5/app.cpp
    src/ps5/audio.cpp
    src/ps5/input.cpp
    src/ps5/io.cpp
    src/ps5/memory.cpp
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
endif()
