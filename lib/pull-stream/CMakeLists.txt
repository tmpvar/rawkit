cmake_minimum_required(VERSION 3.14)
project(pull-stream)


option(PULL_STREAM_BUILD_TESTS "Build the pull-stream test programs" ON)
option(PULL_STREAM_TEST_COVERAGE "Add coverage information for tests run against libpull_stream" OFF)

include_directories(include)

# DEP: libuv
add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/deps/libuv-1.39.0
)
set(LIBUV_LIBS uv_a ${uv_libraries} CACHE INTERNAL "")
set(LIBUV_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/deps/libuv-1.39.0/include CACHE INTERNAL "")
include_directories(${LIBUV_INCLUDE_DIRS})

file(GLOB PULL_STREAM_CORE_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/src/core/*.c"
)

file(GLOB PULL_STREAM_IO_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/src/io/*.c"
)

file(GLOB PULL_STREAM_PRIMITIVE_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/src/sync/*.c"
)

add_library(pull-stream
  ${PULL_STREAM_CORE_SOURCES}
  ${PULL_STREAM_IO_SOURCES}
  ${PULL_STREAM_PRIMITIVE_SOURCES}
)

source_group("core" FILES ${PS_CORE_SOURCES})
source_group("io" FILES ${PS_IO_SOURCES})
source_group("primitive" FILES ${PS_PRIMITIVE_SOURCES})

set_property(TARGET pull-stream PROPERTY CXX_STANDARD 11)
set_property(TARGET pull-stream PROPERTY C_STANDARD 11)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_include_directories(pull-stream PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps/croaring
)

target_link_libraries(pull-stream PUBLIC uv_a)

if (PULL_STREAM_TEST_COVERAGE)
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

# Testing
file(GLOB PULL_STREAM_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(PULL_STREAM_TEST_FILES ${PULL_STREAM_TEST_FILES} CACHE INTERNAL "")

if (PULL_STREAM_BUILD_TESTS)
  target_link_options(pull-stream PUBLIC
    --coverage
  )

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

  target_compile_options(pull-stream-test PRIVATE
    -Wall
  )

  target_link_libraries(pull-stream-test PRIVATE
    pull-stream
  )

  target_include_directories(pull-stream-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  add_test(pull-stream pull-stream-test)
endif()