cmake_minimum_required(VERSION 3.14)
project(rawkit-gcode-parser)


option(RAWKIT_GCODE_PARSER_BUILD_TESTS "Build the pull-stream test programs" ON)

include_directories(include)


# Testing
file(GLOB RAWKIT_GCODE_PARSER_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.h"
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_GCODE_PARSER_TEST_FILES ${RAWKIT_GCODE_PARSER_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_GCODE_PARSER_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_executable(gcode-parser-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${PULL_STREAM_TEST_FILES}
  )

  source_group("tests" FILES ${PULL_STREAM_TEST_FILES})

  target_include_directories(gcode-parser-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  add_test(rawkit-gcode-parser gcode-parser-test)
endif()