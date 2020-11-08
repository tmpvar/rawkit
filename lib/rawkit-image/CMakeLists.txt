cmake_minimum_required(VERSION 3.17)
project(rawkit-image)

option(RAWKIT_IMAGE_BUILD_TESTS "Build the rawkit-image test programs" ON)
option(RAWKIT_IMAGE_TEST_COVERAGE "Add coverage information for tests run against rawkit-image" OFF)

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
  src/rawkit-image.cpp
)

add_library(rawkit::image ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    rawkit::core
    rawkit::file
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/image.h
  DESTINATION include/rawkit
)

install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/stb_image.h
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
if (RAWKIT_IMAGE_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()