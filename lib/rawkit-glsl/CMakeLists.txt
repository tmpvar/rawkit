cmake_minimum_required(VERSION 3.14)
project(rawkit-glsl)


option(RAWKIT_GLSL_BUILD_TESTS "Build the rawkit-glsl test programs" ON)
option(RAWKIT_GLSL_TEST_COVERAGE "Add coverage information for tests run against libpull_stream" OFF)

add_library(rawkit-glsl
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-glsl.cpp
)

# DEP: pull-stream (sibling)
if (NOT PULL_STREAM_LIBS)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()

target_include_directories(rawkit-glsl PUBLIC
  ${PULL_STREAM_INCLUDES}
)

target_link_libraries(rawkit-glsl PUBLIC
  ${PULL_STREAM_LIBS}
)

# DEP: glslang
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
# set(ENABLE_PCH OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/glslang)

target_include_directories(rawkit-glsl PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps
  ${CMAKE_CURRENT_LIST_DIR}/deps/glslang
)

target_compile_options(rawkit-glsl PRIVATE
  -DUNICODE
  -D_UNICODE
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
  # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
  -DNOMINMAX=1
)

set_property(TARGET rawkit-glsl PROPERTY CXX_STANDARD 17)

target_include_directories(rawkit-glsl PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(rawkit-glsl PRIVATE
  glslang
  SPIRV
)

if (RAWKIT_GLSL_TEST_COVERAGE)
  if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    target_compile_options(rawkit-glsl PUBLIC
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
file(GLOB RAWKIT_GLSL_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_GLSL_TEST_FILES ${RAWKIT_GLSL_TEST_FILES} CACHE INTERNAL "")

if (RAWKIT_GLSL_BUILD_TESTS)
  # DEP: doctest
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  include(CTest)
  enable_testing()

  add_executable(rawkit-glsl-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${RAWKIT_GLSL_TEST_FILES}
  )

  set_property(TARGET rawkit-glsl-test PROPERTY CXX_STANDARD 17)

  source_group("tests" FILES ${RAWKIT_GLSL_TEST_FILES})

  target_link_libraries(rawkit-glsl-test PRIVATE
    rawkit-glsl
    ${PULL_STREAM_LIBS}
  )

  target_include_directories(rawkit-glsl-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
    ${PULL_STREAM_INCLUDES}
  )

  add_test(rawkit-glsl rawkit-glsl-test)
endif()