cmake_minimum_required(VERSION 3.14)
project(rawkit-hot)


option(RAWKIT_HOT_BUILD_TESTS "Build the rawkit-hot test programs" ON)
option(RAWKIT_HOT_TEST_COVERAGE "Add coverage information for tests run against librawkit-hot" OFF)

add_library(rawkit-hot
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-hot.cpp
)

# DEP: pull-stream (sibling)
if (NOT RAWKIT_HASH_LIBS)
  set(RAWKIT_HASH_BUILD_TESTS OFF)
  add_subdirectory(../rawkit-hash deps/rawkit-hash)
endif()

target_include_directories(rawkit-hot PUBLIC
  ${RAWKIT_HASH_INCLUDES}
)

target_link_libraries(rawkit-hot PUBLIC
  ${RAWKIT_HASH_LIBS}
)

target_compile_options(rawkit-hot PRIVATE
  -DUNICODE
  -D_UNICODE
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
  # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
  -DNOMINMAX=1
)

set_property(TARGET rawkit-hot PROPERTY CXX_STANDARD 17)

set(RAWKIT_HOT_INCLUDES
  ${CMAKE_CURRENT_LIST_DIR}/include
)
set(RAWKIT_HOT_INCLUDES ${RAWKIT_HOT_INCLUDES} PARENT_SCOPE)

set(RAWKIT_HOT_LIBS ${RAWKIT_HOT_LIBS} rawkit-hot)
set(RAWKIT_HOT_LIBS rawkit-hot PARENT_SCOPE)

target_include_directories(rawkit-hot PUBLIC
  ${RAWKIT_HOT_INCLUDES}
)

target_include_directories(rawkit-hot PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/internal
)

if (RAWKIT_HOT_TEST_COVERAGE)
  if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_compile_options(rawkit-hot PUBLIC
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
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/hot.h
  DESTINATION include/rawkit
)


# Testing
file(GLOB RAWKIT_HOT_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_HOT_TEST_FILES ${RAWKIT_HOT_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_HOT_BUILD_TESTS)
  # DEP: doctest
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  include(CTest)
  enable_testing()

  add_executable(rawkit-hot-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${RAWKIT_HOT_TEST_FILES}
  )

  set_property(TARGET rawkit-hot-test PROPERTY CXX_STANDARD 17)

  source_group("tests" FILES ${RAWKIT_HOT_TEST_FILES})

  target_link_libraries(rawkit-hot-test PUBLIC
    ${RAWKIT_HOT_LIBS}
    ${RAWKIT_HASH_LIBS}
  )

  target_include_directories(rawkit-hot-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
    ${RAWKIT_HOT_INCLUDES}
  )

  add_test(rawkit-hot rawkit-hot-test)
endif()