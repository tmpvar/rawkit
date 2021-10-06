cmake_minimum_required(VERSION 3.17)
project(rawkit-worker)

# LIB: rawkit-core
if (NOT TARGET rawkit::core)
  add_subdirectory(../rawkit-core rawkit-core)
endif()

# LIB: rawkit-jit
if (NOT TARGET rawkit::jit)
  add_subdirectory(../rawkit-jit rawkit-jit)
endif()

# LIB: rawkit-gpu
if (NOT TARGET rawkit::gpu)
  add_subdirectory(../rawkit-gpu rawkit-gpu)
endif()

# LIB: rawkit-vulkan
if (NOT TARGET rawkit::vulkan)
  add_subdirectory(../rawkit-vulkan rawkit-vulkan)
endif()

add_library(${PROJECT_NAME} STATIC
  src/rawkit-worker.cpp
)

add_library(rawkit::worker ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rawkit::core
    rawkit::jit
    rawkit::gpu
    rawkit::vulkan
)


# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/worker.h
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
