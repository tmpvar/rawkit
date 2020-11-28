cmake_minimum_required(VERSION 3.17)
project(rawkit-texture)

option(RAWKIT_TEXTURE_BUILD_TESTS "Build the rawkit-texture test programs" ON)
option(RAWKIT_TEXTURE_TEST_COVERAGE "Add coverage information for tests run against rawkit-texture" OFF)

# DEP: rawkit-core (sibling)
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core deps/rawkit-core)
endif()

# DEP: rawkit-gpu (sibling)
if (NOT TARGET rawkit::gpu)
  add_subdirectory(../rawkit-gpu deps/rawkit-gpu)
endif()

# DEP: rawkit-hash (sibling)
if (NOT TARGET rawkit::hash)
  add_subdirectory(../rawkit-hash deps/rawkit-hash)
endif()

# DEP: rawkit-hot (sibling)
if (NOT TARGET rawkit::hot)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()

# DEP: rawkit-image (sibling)
if (NOT TARGET rawkit::image)
  add_subdirectory(../rawkit-image deps/rawkit-image)
endif()

# DEP: rawkit-window (sibling)
if (NOT TARGET rawkit::window)
  add_subdirectory(../rawkit-window deps/rawkit-window)
endif()

add_library(${PROJECT_NAME} STATIC
  src/rawkit-texture.cpp
  src/texture-target.cpp
)

add_library(rawkit::texture ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    pull::stream
    rawkit::core
    rawkit::gpu
    rawkit::hot
    rawkit::image
    rawkit::window
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/texture.h
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
if (RAWKIT_TEXTURE_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()