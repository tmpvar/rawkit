cmake_minimum_required(VERSION 3.14)
project(rawkit-core)

option(RAWKIT_CORE_BUILD_TESTS "Build the rawkit-core test programs" ON)
option(RAWKIT_CORE_TEST_COVERAGE "Add coverage information for tests run against rawkit-core" OFF)

add_library(${PROJECT_NAME} STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-core.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-args.cpp
)

add_library(rawkit::core ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_17
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deps/flags/include>
    $<INSTALL_INTERFACE:include>
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/core.h
  DESTINATION include/rawkit
)

install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/termcolor.h
  DESTINATION include
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
if (RAWKIT_CORE_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()