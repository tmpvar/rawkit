cmake_minimum_required(VERSION 3.14)
project(rawkit-file)


option(RAWKIT_FILE_BUILD_TESTS "Build the rawkit-file test programs" ON)
option(RAWKIT_FILE_TEST_COVERAGE "Add coverage information for tests run against librawkit-file" OFF)

add_library(rawkit-file
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-file.cpp
)

# DEP: pull-stream (sibling)
if (NOT PULL_STREAM_LIBS)
  set(PULL_STREAM_BUILD_TESTS OFF)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()
target_include_directories(rawkit-file PUBLIC
  ${PULL_STREAM_INCLUDES}
)

# DEP: rawkit-hot (sibling)
if (NOT RAWKIT_HOT_LIBS)
  set(RAWKIT_HOT_BUILD_TESTS OFF)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()
target_include_directories(rawkit-file PUBLIC
  ${RAWKIT_HOT_INCLUDES}
)

target_include_directories(rawkit-file PUBLIC
  ${RAWKIT_HASH_INCLUDES}
)

target_link_libraries(rawkit-file PUBLIC
  ${RAWKIT_HASH_LIBS}
)

target_compile_options(rawkit-file PRIVATE
  -DUNICODE
  -D_UNICODE
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
  # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
  -DNOMINMAX=1
)

set_property(TARGET rawkit-file PROPERTY CXX_STANDARD 17)

set(RAWKIT_FILE_INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
)
set(RAWKIT_FILE_INCLUDES ${RAWKIT_FILE_INCLUDES} PARENT_SCOPE)

set(RAWKIT_FILE_LIBS
  rawkit-file
  ${PULL_STREAM_LIBS}
  ${RAWKIT_HOT_LIBS}
)
set(RAWKIT_FILE_LIBS rawkit-file PARENT_SCOPE)

target_include_directories(rawkit-file PUBLIC
  ${RAWKIT_FILE_INCLUDES}
)

target_include_directories(rawkit-file PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/internal
)

if (RAWKIT_FILE_TEST_COVERAGE)
  if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_compile_options(rawkit-file PUBLIC
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
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/file.h
  DESTINATION include/rawkit
)


# Testing
file(GLOB RAWKIT_FILE_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_FILE_TEST_FILES ${RAWKIT_FILE_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_FILE_BUILD_TESTS)
  # DEP: doctest
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  include(CTest)
  enable_testing()

  add_executable(rawkit-file-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${RAWKIT_FILE_TEST_FILES}
  )

  set_property(TARGET rawkit-file-test PROPERTY CXX_STANDARD 17)

  source_group("tests" FILES ${RAWKIT_FILE_TEST_FILES})

  target_link_libraries(rawkit-file-test PUBLIC
    ${RAWKIT_FILE_LIBS}
    ${RAWKIT_HASH_LIBS}
  )

  target_include_directories(rawkit-file-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
    ${RAWKIT_FILE_INCLUDES}
  )

  add_test(rawkit-file rawkit-file-test)
endif()