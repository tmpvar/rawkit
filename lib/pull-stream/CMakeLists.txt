cmake_minimum_required(VERSION 3.14)
project(pull-stream VERSION 1.0)

option(PULL_STREAM_BUILD_TESTS "Build the pull-stream test programs" ON)
option(PULL_STREAM_TEST_COVERAGE "Add coverage information for tests run against libpull_stream" OFF)

# DEP: libuv
add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/deps/libuv
)

add_library(${PROJECT_NAME}
  # utils
  ${CMAKE_CURRENT_LIST_DIR}/src/core/util.c

  # io
  ${CMAKE_CURRENT_LIST_DIR}/src/io/duplex-tcp.c
  ${CMAKE_CURRENT_LIST_DIR}/src/io/server-tcp.c
  ${CMAKE_CURRENT_LIST_DIR}/src/io/sink-file.c
  ${CMAKE_CURRENT_LIST_DIR}/src/io/source-file.c
  ${CMAKE_CURRENT_LIST_DIR}/src/io/util.c
  ${CMAKE_CURRENT_LIST_DIR}/src/io/uv-duplex.c

  # sync
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/collector.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/counter.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/hex-printer.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/multiplier.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/nooper.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/reverser.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/single-value.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/splitter.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/taker.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sync/user-value.c
)

add_library(pull::stream ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    uv::uv_a
)

install(DIRECTORY include/pull DESTINATION include)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include
)

install(EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE pull::
  DESTINATION lib/cmake/Pull
)

# Testing
if (PULL_STREAM_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()