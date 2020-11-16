cmake_minimum_required(VERSION 3.17)
project(rawkit-window)

option(RAWKIT_WINDOW_BUILD_TESTS "Build the rawkit-window test programs" ON)
option(RAWKIT_WINDOW_TEST_COVERAGE "Add coverage information for tests run against rawkit-window" OFF)

# DEP: rawkit-core (sibling)
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core deps/rawkit-core)
endif()

# DEP: rawkit-gpu (sibling)
if (NOT TARGET rawkit::gpu)
  add_subdirectory(../rawkit-gpu deps/rawkit-gpu)
endif()

# DEP: rawkit-hot (sibling)
if (NOT TARGET rawkit::hot)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()

# DEP: glfw
if (NOT TARGET glfw::glfw)
  add_subdirectory(deps/glfw)
endif()


add_library(${PROJECT_NAME} STATIC
  src/rawkit-window.cpp
)

add_library(rawkit::window ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    rawkit::core
    rawkit::gpu
    rawkit::hot
    glfw::glfw
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/window.h
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
if (RAWKIT_WINDOW_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()