cmake_minimum_required(VERSION 3.14)
project(rawkit-hot)

option(RAWKIT_HOT_BUILD_TESTS "Build the rawkit-hot test programs" ON)
option(RAWKIT_HOT_TEST_COVERAGE "Add coverage information for tests run against librawkit-hot" OFF)

# DEP: rawkit-hash (sibling)
if (NOT TARGET rawkit::hash)
  add_subdirectory(../rawkit-hash deps/rawkit-hash)
endif()

add_library(rawkit-hot STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-hot.cpp
)

add_library(rawkit::hot ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    rawkit::hash
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_14
)

target_compile_options(
  ${PROJECT_NAME}
  PRIVATE
    -DUNICODE
    -D_UNICODE
    -D__STDC_CONSTANT_MACROS
    -D__STDC_FORMAT_MACROS
    -D__STDC_LIMIT_MACROS
    # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
    -DNOMINMAX=1
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/hot.h
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
if (RAWKIT_HOT_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()