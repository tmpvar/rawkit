cmake_minimum_required(VERSION 3.14)
project(rawkit-gpu)

option(RAWKIT_GPU_BUILD_TESTS "Build the rawkit-gpu test programs" ON)
option(RAWKIT_GPU_TEST_COVERAGE "Add coverage information for tests run against librawkit-gpu" OFF)

# DEP: rawkit-core (sibling)
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core deps/rawkit-core)
endif()

# DEP: rawkit-hash (sibling)
if (NOT TARGET rawkit::hash)
  add_subdirectory(../rawkit-hash deps/rawkit-hash)
endif()

# DEP: rawkit-hot (sibling)
if (NOT TARGET rawkit::hot)
  add_subdirectory(../rawkit-hot deps/rawkit-hot)
endif()

# DEP: rawkit-mesh (sibling)
if (NOT TARGET rawkit::mesh)
  add_subdirectory(../rawkit-mesh deps/rawkit-mesh)
endif()


# DEP: rawkit-vulkan
add_subdirectory(deps/vulkan)

add_library(${PROJECT_NAME} STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/rawkit-gpu.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/init-vulkan.cpp
)

add_library(rawkit::gpu ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rawkit::hash
    rawkit::hot

  PUBLIC
    rawkit::mesh
    rawkit::vulkan
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_14
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/gpu.h
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

# # Testing
# if (RAWKIT_GPU_BUILD_TESTS)
#   include(CTest)
#   enable_testing()
#   add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
# endif()