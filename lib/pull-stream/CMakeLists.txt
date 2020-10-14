cmake_minimum_required(VERSION 3.14)
project(pull-stream)

option(PULL_STREAM_BUILD_TESTS "Build the pull-stream test programs" ON)
option(PULL_STREAM_TEST_COVERAGE "Add coverage information for tests run against libpull_stream" OFF)




file(GLOB PULL_STREAM_CORE_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/src/core/*.c"
)

file(GLOB PULL_STREAM_IO_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/src/io/*.c"
)

file(GLOB PULL_STREAM_SYNC_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/src/sync/*.c"
)

add_library(pull-stream
  ${PULL_STREAM_CORE_SOURCES}
  ${PULL_STREAM_IO_SOURCES}
  ${PULL_STREAM_SYNC_SOURCES}
)

set(PULL_STREAM_INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/deps/libuv-1.39.0/include
)

target_include_directories(pull-stream PUBLIC
  ${PULL_STREAM_INCLUDES}
)

# DEP: libuv
add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/deps/libuv-1.39.0
)

set(PULL_STREAM_INCLUDES ${PULL_STREAM_INCLUDES} PARENT_SCOPE)
set(PULL_STREAM_LIBS pull-stream PARENT_SCOPE)

source_group("core" FILES ${PS_CORE_SOURCES})
source_group("io" FILES ${PS_IO_SOURCES})
source_group("sync" FILES ${PS_PRIMITIVE_SOURCES})

set_property(TARGET pull-stream PROPERTY CXX_STANDARD 11)
set_property(TARGET pull-stream PROPERTY C_STANDARD 11)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(pull-stream PUBLIC uv_a)

if (PULL_STREAM_TEST_COVERAGE)
  if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_compile_options(pull-stream PUBLIC
      -g
      -O0
      --coverage
      -ftest-coverage
      -fprofile-arcs
      -fno-default-inline
      -fno-inline
    )
  endif()
endif()

# Testing
file(GLOB PULL_STREAM_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(PULL_STREAM_TEST_FILES ${PULL_STREAM_TEST_FILES} CACHE INTERNAL "")

if (PULL_STREAM_BUILD_TESTS)
  # DEP: doctest
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  include(CTest)
  enable_testing()

  add_executable(pull-stream-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${PULL_STREAM_TEST_FILES}
  )

  set_property(TARGET pull-stream-test PROPERTY CXX_STANDARD 11)
  set_property(TARGET pull-stream-test PROPERTY C_STANDARD 11)

  source_group("tests" FILES ${PULL_STREAM_TEST_FILES})

  target_link_libraries(pull-stream-test PRIVATE
    pull-stream
  )

  target_include_directories(pull-stream-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  add_test(pull-stream pull-stream-test)
endif()