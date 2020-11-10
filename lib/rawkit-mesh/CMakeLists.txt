cmake_minimum_required(VERSION 3.17)
project(rawkit-mesh)

option(RAWKIT_MESH_BUILD_TESTS "Build the rawkit-mesh test programs" ON)
option(RAWKIT_MESH_TEST_COVERAGE "Add coverage information for tests run against rawkit-mesh" OFF)

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

# DEP: rawkit-hot (sibling)
if (NOT TARGET rawkit::hot)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()

add_library(${PROJECT_NAME} STATIC
  src/rawkit-mesh.cpp
)

add_library(rawkit::mesh ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    pull::stream
    rawkit::core
    rawkit::hot
    rawkit::file
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_14
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/mesh.h
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
if (RAWKIT_MESH_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()