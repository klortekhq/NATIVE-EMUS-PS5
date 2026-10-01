add_library(corehost STATIC
  src/static_core.cpp
  src/ps5rt_bridge.cpp
  src/linked_core.cpp
  src/ps5_runner.cpp
  src/vfs.cpp
)
add_library(native_emus::corehost ALIAS corehost)

target_include_directories(corehost PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(corehost PUBLIC cxx_std_20)
target_link_libraries(corehost PUBLIC ps5rt::ps5rt ps5rt::common)

option(COREHOST_BUILD_TESTS "Build static-core host tests" OFF)
if(COREHOST_BUILD_TESTS)
  add_executable(corehost-static-test tests/static_core_test.cpp)
  target_link_libraries(corehost-static-test PRIVATE native_emus::corehost)
  add_test(NAME corehost-static COMMAND corehost-static-test)

  add_executable(corehost-ps5rt-bridge-test tests/ps5rt_bridge_test.cpp)
  target_link_libraries(corehost-ps5rt-bridge-test PRIVATE native_emus::corehost)
  add_test(NAME corehost-ps5rt-bridge COMMAND corehost-ps5rt-bridge-test)

  add_executable(corehost-vfs-test tests/vfs_test.cpp)
  target_link_libraries(corehost-vfs-test PRIVATE native_emus::corehost)
  add_test(NAME corehost-vfs COMMAND corehost-vfs-test)
endif()
