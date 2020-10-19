cmake_minimum_required(VERSION 3.14)
project(rawkit-diskwatcher)

option(RAWKIT_DISKWATCHER_BUILD_TESTS "Build the rawkit-diskwatcher test programs" ON)
option(RAWKIT_DISKWATCHER_TEST_COVERAGE "Add coverage information for tests run against librawkit-diskwatcher" OFF)

# DEP: pull-stream (sibling)
if (NOT TARGET pull::stream)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()

add_library(${PROJECT_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-diskwatcher.cpp
)

add_library(rawkit::diskwatcher ALIAS ${PROJECT_NAME})

target_compile_options(${PROJECT_NAME}
  PRIVATE
    -DUNICODE
    -D_UNICODE
    -D__STDC_CONSTANT_MACROS
    -D__STDC_FORMAT_MACROS
    -D__STDC_LIMIT_MACROS
    # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
    -DNOMINMAX=1
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_14
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  pull::stream
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/diskwatcher.h
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
if (RAWKIT_DISKWATCHER_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()