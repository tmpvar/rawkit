cmake_minimum_required(VERSION 3.14)
project(rawkit-spirv-cross)

option(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS "Instead of throwing exceptions assert" OFF)
set(SPIRV_CROSS_SHARED OFF)
set(SPIRV_CROSS_STATIC ON)
set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)

set(SPIRV_CROSS_ENABLE_GLSL ON)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_REFLECT ON)
set(SPIRV_CROSS_ENABLE_C_API ON)
set(SPIRV_CROSS_ENABLE_UTIL ON)

add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/SPIRV-Cross-a57b4b1b2ef6f7b31b27588f0e7288c630d68c08
)

if(NOT TARGET SPIRV::Cross)
  add_library(${PROJECT_NAME} INTERFACE)
  add_library(SPIRV::Cross ALIAS ${PROJECT_NAME})

  target_link_libraries(${PROJECT_NAME}
    INTERFACE
      spirv-cross-glsl
      spirv-cross-reflect
      spirv-cross-util
      spirv-cross-core
  )

  install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
  )

  install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE SPIRV::
    DESTINATION lib/cmake/SPIRV
  )
endif()
