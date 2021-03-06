cmake_minimum_required(VERSION 3.17)
project(rawkit-shader)

option(RAWKIT_SHADER_BUILD_TESTS "Build the rawkit-shader test programs" ON)
option(RAWKIT_SHADER_TEST_COVERAGE "Add coverage information for tests run against rawkit-shader" OFF)

# DEP: pull-stream (sibling)
if (NOT TARGET pull::stream)
  add_subdirectory(../pull-stream deps/pull-stream)
endif()

# DEP: rawkit-core (sibling)
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core deps/rawkit-core)
endif()

# DEP: rawkit-file (sibling)
if (NOT TARGET rawkit::file)
  add_subdirectory(../rawkit-file deps/rawkit-file)
endif()

# DEP: rawkit-glsl (sibling)
if (NOT TARGET rawkit::glsl)
  add_subdirectory(../rawkit-glsl deps/rawkit-glsl)
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

# DEP: rawkit-texture (sibling)
if (NOT TARGET rawkit::texture)
  add_subdirectory(../rawkit-texture deps/rawkit-texture)
endif()

add_library(${PROJECT_NAME} STATIC
  src/rawkit-shader.cpp
  src/shader-state.cpp
  src/pipeline-compute.cpp
  src/pipeline-graphics.cpp
  src/pipeline-layout.cpp
  src/instance.cpp
  src/params.cpp
)

add_library(rawkit::shader ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    pull::stream
    rawkit::core
    rawkit::file
    rawkit::gpu
    rawkit::glsl
    rawkit::hash
    rawkit::hot
    rawkit::texture
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/shader.h
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
if (RAWKIT_SHADER_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()