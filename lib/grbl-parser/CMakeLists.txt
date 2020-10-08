cmake_minimum_required(VERSION 3.14)
project(rawkit-gcode-parser)


option(RAWKIT_GRBL_PARSER_BUILD_TESTS "Build the pull-stream test programs" ON)

include_directories(include)


# Testing
file(GLOB RAWKIT_GRBL_PARSER_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.h"
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_GRBL_PARSER_TEST_FILES ${RAWKIT_GRBL_PARSER_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_GRBL_PARSER_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_executable(grbl-parser-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${PULL_STREAM_TEST_FILES}
  )

  source_group("tests" FILES ${PULL_STREAM_TEST_FILES})

  target_include_directories(grbl-parser-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  add_test(rawkit-grbl-parser grbl-parser-test)
endif()