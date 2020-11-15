cmake_minimum_required(VERSION 3.17)
project(rawkit-vg)

option(RAWKIT_VG_BUILD_TESTS "Build the rawkit-vg test programs" ON)
option(RAWKIT_VG_TEST_COVERAGE "Add coverage information for tests run against rawkit-vg" OFF)

# DEP: rawkit-core (sibling)
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core deps/rawkit-core)
endif()

# DEP: rawkit-gpu (sibling)
if (NOT TARGET rawkit::gpu)
  add_subdirectory(../rawkit-gpu deps/rawkit-gpu)
endif()

# DEP: rawkit-texture (sibling)
if (NOT TARGET rawkit::texture)
  add_subdirectory(../rawkit-texture deps/rawkit-texture)
endif()

add_library(${PROJECT_NAME} STATIC
  src/nanovg/nanovg.c
  src/nanovg/nanovg_vk.c
  src/rawkit-vg.cpp
  src/nanovg-binding.cpp
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
   -DNVG_NO_STB
   -DNANOVG_VULKAN_IMPLEMENTATION
)

add_library(rawkit::vg ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    rawkit::core
    rawkit::gpu
    rawkit::texture
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/vg.h
  DESTINATION include/rawkit
)

install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/nanovg/nanovg.h
  DESTINATION include/nanovg
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
if (RAWKIT_VG_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()