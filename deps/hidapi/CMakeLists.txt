cmake_minimum_required(VERSION 3.1.4)
project(hidapi)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(HIDAPI_DIR "${CMAKE_CURRENT_LIST_DIR}/hidapi-8741697")

add_subdirectory(${HIDAPI_DIR})

install(
  DIRECTORY
    ${HIDAPI_DIR}/hidapi
  DESTINATION include
)