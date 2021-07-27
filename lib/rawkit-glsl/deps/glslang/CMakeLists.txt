
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
# set(ENABLE_PCH OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)

add_subdirectory(
  # ${CMAKE_CURRENT_LIST_DIR}/glslang-SDK-candidate-26-Jul-2020
  ${CMAKE_CURRENT_LIST_DIR}/glslang-11.5.0
)

list(APPEND GLSLANG_EXPORTS
  glslang
  SPIRV
  OGLCompiler
  OSDependent
  MachineIndependent
  GenericCodeGen
)

foreach(GLSL_LIB IN LISTS GLSLANG_EXPORTS)
  add_library(glslang::${GLSL_LIB} ALIAS ${GLSL_LIB})
  install(TARGETS ${GLSL_LIB}
    EXPORT ${GLSL_LIB}Targets
  )
  install(EXPORT ${GLSL_LIB}Targets
    FILE ${GLSL_LIB}Targets.cmake
    NAMESPACE glslang::
    DESTINATION lib/cmake/glslang
  )
endforeach()
