cmake_minimum_required(VERSION 3.20)

project(ps5rt LANGUAGES C CXX)

add_library(ps5rt INTERFACE)
add_library(ps5rt::ps5rt ALIAS ps5rt)

target_compile_features(ps5rt INTERFACE cxx_std_20)

target_include_directories(ps5rt
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Portable host services that are shared by native PS5 and host tests.
add_library(ps5rt_common STATIC
  src/common/io.cpp
  src/common/input.cpp
  src/common/thread.cpp
)
add_library(ps5rt::common ALIAS ps5rt_common)
target_link_libraries(ps5rt_common PUBLIC ps5rt::ps5rt)
target_compile_features(ps5rt_common PUBLIC cxx_std_20)

# The concrete PS5 implementation will live behind this public contract.
# Emulator cores should depend on ps5rt::ps5rt / ps5rt::common rather than on
# another emulator/frontend.

option(PS5RT_BUILD_HEADER_TEST "Build the ps5rt public-header smoke test" OFF)

if(PS5RT_BUILD_HEADER_TEST)
  add_executable(ps5rt-header-test tests/header_compile.cpp)
  target_link_libraries(ps5rt-header-test PRIVATE ps5rt::ps5rt)

  add_executable(ps5rt-media-layout-test tests/media_layout_test.cpp)
  target_link_libraries(ps5rt-media-layout-test PRIVATE ps5rt::ps5rt)
  add_test(NAME ps5rt-media-layout COMMAND ps5rt-media-layout-test)

  add_executable(ps5rt-io-backend-test tests/io_backend_test.cpp)
  target_link_libraries(ps5rt-io-backend-test PRIVATE ps5rt::common)
  add_test(NAME ps5rt-io-backend COMMAND ps5rt-io-backend-test)

  add_executable(ps5rt-input-mapping-test tests/input_mapping_test.cpp)
  target_link_libraries(ps5rt-input-mapping-test PRIVATE ps5rt::common)
  add_test(NAME ps5rt-input-mapping COMMAND ps5rt-input-mapping-test)

  add_executable(ps5rt-thread-policy-test tests/thread_policy_test.cpp)
  target_link_libraries(ps5rt-thread-policy-test PRIVATE ps5rt::common)
  add_test(NAME ps5rt-thread-policy COMMAND ps5rt-thread-policy-test)
endif()

option(PS5RT_BUILD_PS5_BACKEND "Build the native PS5 backend implementation" OFF)

if(PS5RT_BUILD_PS5_BACKEND)
  add_library(ps5rt_ps5 STATIC
    src/ps5/app.cpp
    src/ps5/audio.cpp
    src/ps5/diagnostics.cpp
    src/ps5/input.cpp
    src/ps5/input_hid.cpp
    src/ps5/log.cpp
    src/ps5/memory.cpp
    src/ps5/thread.cpp
    src/ps5/vfs.cpp
    src/ps5/video.cpp
  )
  add_library(ps5rt::ps5 ALIAS ps5rt_ps5)
  target_link_libraries(ps5rt_ps5 PUBLIC ps5rt::ps5rt ps5rt::common)
  target_compile_features(ps5rt_ps5 PUBLIC cxx_std_20)
  option(PS5RT_ENABLE_SMB "Enable the read-only SMB2/SMB3 ps5rt backend" OFF)
  option(PS5RT_ENABLE_EMU_SERVER "Enable emus:// random access through SERVER-EMUS-PS5" OFF)
  set(PS5RT_LIBSMB2_INCLUDE_DIR "" CACHE PATH "Directory containing smb2/libsmb2.h")
  set(PS5RT_LIBSMB2_LIBRARY "" CACHE FILEPATH "Path to static libsmb2.a")

  if(PS5RT_ENABLE_EMU_SERVER)
    target_sources(ps5rt_ps5 PRIVATE src/ps5/io_emu_server.cpp)
    target_link_libraries(ps5rt_ps5 PRIVATE SceNet SceSsl SceHttp)
    target_compile_definitions(ps5rt_ps5 PRIVATE PS5RT_HAS_EMU_SERVER=1)
  endif()

  if(PS5RT_ENABLE_SMB)
    if(NOT EXISTS "${PS5RT_LIBSMB2_INCLUDE_DIR}/smb2/libsmb2.h")
      message(FATAL_ERROR "PS5RT_ENABLE_SMB requires PS5RT_LIBSMB2_INCLUDE_DIR")
    endif()
    if(NOT EXISTS "${PS5RT_LIBSMB2_LIBRARY}")
      message(FATAL_ERROR "PS5RT_ENABLE_SMB requires PS5RT_LIBSMB2_LIBRARY")
    endif()
    target_sources(ps5rt_ps5 PRIVATE src/ps5/io_smb.cpp)
    target_include_directories(ps5rt_ps5 PRIVATE "${PS5RT_LIBSMB2_INCLUDE_DIR}")
    target_link_libraries(ps5rt_ps5 PRIVATE "${PS5RT_LIBSMB2_LIBRARY}" SceNet)
    target_compile_definitions(ps5rt_ps5 PRIVATE PS5RT_HAS_SMB=1)
  endif()

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

    add_executable(ps5rt-flycast-jit-probe probes/flycast_jit_probe.cpp)
    target_link_libraries(ps5rt-flycast-jit-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-flycast-jit-probe PRIVATE cxx_std_20)

    add_executable(ps5rt-rpcs3-sparse-jit-probe probes/rpcs3_sparse_jit_probe.cpp)
    target_link_libraries(ps5rt-rpcs3-sparse-jit-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-rpcs3-sparse-jit-probe PRIVATE cxx_std_20)

    add_executable(ps5rt-pool-memory-probe probes/pool_memory_probe.cpp)
    target_link_libraries(ps5rt-pool-memory-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-pool-memory-probe PRIVATE cxx_std_20)

    add_executable(ps5rt-memory-diagnostics-probe probes/memory_diagnostics_probe.cpp)
    target_link_libraries(ps5rt-memory-diagnostics-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-memory-diagnostics-probe PRIVATE cxx_std_20)

    add_executable(ps5rt-thread-probe probes/thread_probe.cpp)
    target_link_libraries(ps5rt-thread-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-thread-probe PRIVATE cxx_std_20)

    add_executable(ps5rt-emutls-probe probes/emutls_probe.cpp)
    target_link_libraries(ps5rt-emutls-probe PRIVATE ps5rt::ps5)
    target_compile_features(ps5rt-emutls-probe PRIVATE cxx_std_20)
    target_compile_options(ps5rt-emutls-probe PRIVATE -femulated-tls)
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

  add_executable(ps5rt-diagnostics-host-test
    tests/diagnostics_host_test.cpp
    src/ps5/diagnostics.cpp
  )
  target_link_libraries(ps5rt-diagnostics-host-test PRIVATE ps5rt::ps5rt)
  target_compile_features(ps5rt-diagnostics-host-test PRIVATE cxx_std_20)
  add_test(NAME ps5rt-diagnostics-host COMMAND ps5rt-diagnostics-host-test)

  add_executable(ps5rt-emu-server-uri-host-test
    tests/emu_server_uri_test.cpp
  )
  target_compile_features(ps5rt-emu-server-uri-host-test PRIVATE cxx_std_20)
  add_test(NAME ps5rt-emu-server-uri-host COMMAND ps5rt-emu-server-uri-host-test)
endif()
