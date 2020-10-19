cmake_minimum_required(VERSION 3.14)
project(UV VERSION 1.39.0)

add_subdirectory(libuv-1.39.0)

add_library(uv::uv_a ALIAS uv_a)

install(TARGETS uv_a
  EXPORT UVStaticTargets
)

install(EXPORT UVStaticTargets
  FILE UVStaticTargets.cmake
  NAMESPACE uv::
  DESTINATION lib/cmake/uv
)