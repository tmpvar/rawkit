cmake_minimum_required(VERSION 3.14)
project(rawkit-diskwatcher)


option(RAWKIT_DISKWATCHER_BUILD_TESTS "Build the rawkit-diskwatcher test programs" ON)
option(RAWKIT_DISKWATCHER_TEST_COVERAGE "Add coverage information for tests run against librawkit-diskwatcher" OFF)

add_library(rawkit-diskwatcher
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-diskwatcher.cpp
)

# DEP: pull-stream (sibling)
if (NOT PULL_STREAM_LIBS)
  set(PULL_STREAM_BUILD_TESTS OFF)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()
target_include_directories(rawkit-diskwatcher PUBLIC
  ${PULL_STREAM_INCLUDES}
)

# DEP: rawkit-hot (sibling)
if (NOT RAWKIT_HOT_LIBS)
  set(RAWKIT_HOT_BUILD_TESTS OFF)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()
target_include_directories(rawkit-diskwatcher PUBLIC
  ${RAWKIT_HOT_INCLUDES}
)

target_include_directories(rawkit-diskwatcher PUBLIC
  ${RAWKIT_HASH_INCLUDES}
)

target_link_libraries(rawkit-diskwatcher PUBLIC
  ${RAWKIT_HASH_LIBS}
)

target_compile_options(rawkit-diskwatcher PRIVATE
  -DUNICODE
  -D_UNICODE
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
  # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
  -DNOMINMAX=1
)

set_property(TARGET rawkit-diskwatcher PROPERTY CXX_STANDARD 17)

set(RAWKIT_DISKWATCHER_INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
)
set(RAWKIT_DISKWATCHER_INCLUDES ${RAWKIT_DISKWATCHER_INCLUDES} PARENT_SCOPE)

set(RAWKIT_DISKWATCHER_LIBS
  rawkit-diskwatcher
  ${PULL_STREAM_LIBS}
  ${RAWKIT_HOT_LIBS}
)
set(RAWKIT_DISKWATCHER_LIBS rawkit-diskwatcher PARENT_SCOPE)

target_include_directories(rawkit-diskwatcher PUBLIC
  ${RAWKIT_DISKWATCHER_INCLUDES}
)

target_include_directories(rawkit-diskwatcher PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/internal
)

if (RAWKIT_DISKWATCHER_TEST_COVERAGE)
  if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_compile_options(rawkit-diskwatcher PUBLIC
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

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/diskwatcher.h
  DESTINATION include/rawkit
)


# Testing
file(GLOB RAWKIT_DISKWATCHER_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_DISKWATCHER_TEST_FILES ${RAWKIT_DISKWATCHER_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_DISKWATCHER_BUILD_TESTS)
  # DEP: doctest
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  include(CTest)
  enable_testing()

  add_executable(rawkit-diskwatcher-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${RAWKIT_DISKWATCHER_TEST_FILES}
  )

  set_property(TARGET rawkit-diskwatcher-test PROPERTY CXX_STANDARD 17)

  source_group("tests" FILES ${RAWKIT_DISKWATCHER_TEST_FILES})

  target_link_libraries(rawkit-diskwatcher-test PUBLIC
    ${RAWKIT_DISKWATCHER_LIBS}
    ${RAWKIT_HASH_LIBS}
  )

  target_include_directories(rawkit-diskwatcher-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
    ${RAWKIT_DISKWATCHER_INCLUDES}
  )

  add_test(rawkit-diskwatcher rawkit-diskwatcher-test)
endif()