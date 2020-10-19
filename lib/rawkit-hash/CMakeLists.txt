cmake_minimum_required(VERSION 3.14)
project(rawkit-hash)

option(RAWKIT_HASH_BUILD_TESTS "Build the rawkit-hash test programs" ON)
option(RAWKIT_HASH_TEST_COVERAGE "Add coverage information for tests run against librawkit-hash" OFF)

# DEP: pull-stream (sibling)
if (NOT TARGET pull::stream)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()

add_library(rawkit-hash STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-hash.cpp
)

add_library(rawkit::hash ALIAS ${PROJECT_NAME})


target_link_libraries(rawkit-hash PUBLIC
  pull::stream
)

target_compile_options(rawkit-hash
  PRIVATE
    -DUNICODE
    -D_UNICODE
    -D__STDC_CONSTANT_MACROS
    -D__STDC_FORMAT_MACROS
    -D__STDC_LIMIT_MACROS
    # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
    -DNOMINMAX=1
)

if (MSVC)
  target_compile_options(rawkit-hash
    PRIVATE
      -arch:AVX2
  )
else()
  target_compile_options(rawkit-hash
    PRIVATE
      -mavx2
      -maes
  )
endif()


target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/hash.h
  DESTINATION include/rawkit
)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include
)

install(EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE rawkit::
  DESTINATION lib/cmake/rawkit
)

# Testing
if (RAWKIT_HASH_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()