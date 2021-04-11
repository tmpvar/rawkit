cmake_minimum_required(VERSION 3.14)
project(lz4)

set(LZ4_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lz4-1.9.3/lib/)

add_library(${PROJECT_NAME}
  ${LZ4_DIR}/lz4.c
)
add_library(rawkit::lz4 ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${LZ4_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Install
install(
  FILES
    ${LZ4_DIR}/lz4.h
  DESTINATION include/
)