cmake_minimum_required(VERSION 3.17)
project(rawkit-worker)

# LIB: lz4
if (NOT TARGET rawkit::lz4)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/lz4)
endif()

# LIB: hidapi
if (NOT TARGET hidapi::hidapi)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/hidapi)
endif()

# LIB: pull-stream
if (NOT TARGET pull::stream)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pull-stream)
endif()

# LIB: rawkit-core
if (NOT TARGET rawkit::core)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-core)
endif()

# LIB: gcode-parser
if (NOT TARGET rawkit::gcode::parser)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/gcode-parser)
endif()

# LIB: grbl-parser
if (NOT TARGET rawkit::grbl::parser)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/grbl-parser)
endif()

# LIB: rawkit-jit
if (NOT TARGET rawkit::jit)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-jit)
endif()

# LIB: rawkit-glsl
if (NOT TARGET rawkit::glsl)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-glsl)
endif()

# LIB: rawkit-hash
if (NOT TARGET rawkit::glsl)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-hash)
endif()

# LIB: rawkit-hot
if (NOT TARGET rawkit::hot)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-hot)
endif()

# LIB: rawkit-diskwatcher
if (NOT TARGET rawkit::diskwatcher)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-diskwatcher)
endif()

# LIB: rawkit-file
if (NOT TARGET rawkit::file)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-file)
endif()

# LIB: rawkit-gpu
if (NOT TARGET rawkit::gpu)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-gpu)
endif()

# LIB: rawkit-shader
if (NOT TARGET rawkit::shader)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-shader)
endif()

# LIB: rawkit-texture
if (NOT TARGET rawkit::texture)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-texture)
endif()

# LIB: rawkit-image
if (NOT TARGET rawkit::image)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-image)
endif()

# LIB: rawkit-mesh
if (NOT TARGET rawkit::mesh)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-mesh)
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
    rawkit::glsl
    rawkit::hash
    rawkit::hot
    rawkit::file
    rawkit::diskwatcher

    rawkit::grbl::parser
    rawkit::gcode::parser
    rawkit::gpu

    rawkit::shader
    rawkit::texture
    rawkit::image
    rawkit::mesh

    rawkit::vulkan

    # rawkit::lz4
    # hidapi::hidapi
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
