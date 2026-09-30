add_library(corehost STATIC
  src/static_core.cpp
)
add_library(native_emus::corehost ALIAS corehost)

target_include_directories(corehost PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(corehost PUBLIC cxx_std_20)

option(COREHOST_BUILD_TESTS "Build static-core host tests" OFF)
if(COREHOST_BUILD_TESTS)
  add_executable(corehost-static-test tests/static_core_test.cpp)
  target_link_libraries(corehost-static-test PRIVATE native_emus::corehost)
  add_test(NAME corehost-static COMMAND corehost-static-test)
endif()
