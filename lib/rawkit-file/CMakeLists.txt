cmake_minimum_required(VERSION 3.14)
project(rawkit-file)

option(RAWKIT_FILE_BUILD_TESTS "Build the rawkit-file test programs" ON)
option(RAWKIT_FILE_TEST_COVERAGE "Add coverage information for tests run against librawkit-file" OFF)

# DEP: pull-stream (sibling)
if (NOT TARGET pull::stream)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()

# DEP: rawkit-core (sibling)
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core deps/rawkit-core)
endif()

# DEP: rawkit-hot (sibling)
if (NOT TARGET rawkit::hot)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()

# DEP: rawkit-diskwatcher (sibling)
if (NOT TARGET rawkit::diskwatcher)
  add_subdirectory(../rawkit-diskwatcher deps/rawkit-diskwatcher)
endif()

add_library(${PROJECT_NAME} STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-file.cpp
)

add_library(rawkit::file ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC
  rawkit::core
  rawkit::hot
  rawkit::diskwatcher
)

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

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/file.h
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
if (RAWKIT_FILE_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()