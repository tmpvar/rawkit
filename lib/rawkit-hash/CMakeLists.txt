cmake_minimum_required(VERSION 3.14)
project(rawkit-hash)


option(RAWKIT_HASH_BUILD_TESTS "Build the rawkit-hash test programs" ON)
option(RAWKIT_HASH_TEST_COVERAGE "Add coverage information for tests run against librawkit-hash" OFF)

add_library(rawkit-hash
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-hash.cpp
)

# DEP: pull-stream (sibling)
if (NOT PULL_STREAM_LIBS)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()

target_include_directories(rawkit-hash PUBLIC
  ${PULL_STREAM_INCLUDES}
)

target_link_libraries(rawkit-hash PUBLIC
  ${PULL_STREAM_LIBS}
)

target_compile_options(rawkit-hash PRIVATE
  -DUNICODE
  -D_UNICODE
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
  # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
  -DNOMINMAX=1
)

if (MSVC)
  target_compile_options(rawkit-hash PUBLIC
    -arch:AVX2
  )
else()
  target_compile_options(rawkit-hash PUBLIC
    -mavx2
    -maes
  )

endif()

set_property(TARGET rawkit-hash PROPERTY CXX_STANDARD 17)

set(RAWKIT_HASH_INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
)

set(RAWKIT_HASH_INCLUDES ${RAWKIT_HASH_INCLUDES} PARENT_SCOPE)
set(RAWKIT_HASH_LIBS rawkit-hash PARENT_SCOPE)

target_include_directories(rawkit-hash PUBLIC
  ${RAWKIT_HASH_INCLUDES}
)

target_include_directories(rawkit-hash PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/internal
)

if (RAWKIT_HASH_TEST_COVERAGE)
  if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_compile_options(rawkit-hash PUBLIC
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
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/hash.h
  DESTINATION include/rawkit
)


# Testing
file(GLOB RAWKIT_HASH_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_HASH_TEST_FILES ${RAWKIT_HASH_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_HASH_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_executable(rawkit-hash-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${RAWKIT_HASH_TEST_FILES}
  )

  set_property(TARGET rawkit-hash-test PROPERTY CXX_STANDARD 17)

  source_group("tests" FILES ${RAWKIT_HASH_TEST_FILES})



  target_link_libraries(rawkit-hash-test PRIVATE
    ${RAWKIT_HASH_LIBS}
    ${PULL_STREAM_LIBS}
  )

  target_include_directories(rawkit-hash-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
    ${PULL_STREAM_INCLUDES}
    ${RAWKIT_HASH_INCLUDES}
  )

  add_test(rawkit-hash rawkit-hash-test)
endif()