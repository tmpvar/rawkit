cmake_minimum_required(VERSION 3.17)
project(rawkit-jit)

option(RAWKIT_JIT_BUILD_TESTS "Build the rawkit-jit test programs" ON)
option(RAWKIT_JIT_TEST_COVERAGE "Add coverage information for tests run against librawkit_jit" OFF)

# DEP: llvm
set(LLVM_SEARCH_PATHS
  "E:/llvm/" #tmpvar's pc
  # general homebrew install
  "/usr/local/opt/llvm/lib/cmake/llvm"
  "/usr/local/opt/llvm/lib/cmake/clang"
  # ubuntu llvm@10
  /usr/lib/llvm-10
)


find_library(found_LLVM LLVM HINTS ${LLVM_SEARCH_PATHS})
find_package(LLVM REQUIRED CONFIG
  PATHS ${LLVM_SEARCH_PATHS}
  NO_DEFAULT_PATH
)
find_package(Clang REQUIRED CONFIG
  PATHS  ${LLVM_SEARCH_PATHS}
  NO_DEFAULT_PATH
)


add_library(${PROJECT_NAME} STATIC
  src/api.cpp
  src/job.cpp
  src/runnable.cpp
)

add_library(rawkit::jit ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${LLVM_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/deps/whereami/src
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

  set(EXTRA_COMPILE_OPTIONS
    -DUNICODE
    -D_UNICODE
    -D__STDC_CONSTANT_MACROS
    -D__STDC_FORMAT_MACROS
    -D__STDC_LIMIT_MACROS
    # see: https://developercommunity.visualstudio.com/content/problem/93889/error-c2872-byte-ambiguous-symbol.html
    -D_HAS_STD_BYTE=0
    # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
    -DNOMINMAX=1
    # see: https://wiki.nervtech.org/doku.php?id=blog:2020:0418_jit_compiler_part4_crt_dependency
    # -D_MT
    # -D_DLL
  )

endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(EXTRA_LINKER_OPTIONS "-export-dynamic")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
  set(EXTRA_LINKER_OPTIONS "-Wl,-export_dynamic")

  # fix an error on mac:
  #    means the weak symbol cannot be overridden at runtime.
  #    This was likely caused by different translation units
  #    being compiled with different visibility settings.

  function(append_if condition value)
    if (${condition})
      foreach(variable ${ARGN})
        set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
      endforeach(variable)
    endif()
  endfunction()

  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-fvisibility-inlines-hidden" SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
  append_if(SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG "-fvisibility-inlines-hidden" CMAKE_CXX_FLAGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

target_compile_options(rawkit-jit PUBLIC
  ${EXTRA_COMPILE_OPTIONS}
)

target_link_options( ${PROJECT_NAME} PRIVATE
  ${EXTRA_LINKER_OPTIONS}
)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      LLVMCore
      LLVMExecutionEngine
      LLVMMC
      LLVMMCJIT
      LLVMObject
      LLVMOrcJIT
      LLVMOption
      LLVMRuntimeDyld
      LLVMSupport
      LLVMTarget
      LLVMX86CodeGen

      clangBasic
      clangCodeGen
      clangDriver
      clangFrontend
      clangSerialization
  )
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      LLVMCore
      LLVMExecutionEngine
      LLVMMC
      LLVMMCJIT
      LLVMObject
      LLVMOrcJIT
      LLVMOption
      LLVMRuntimeDyld
      LLVMSupport
      LLVMTarget
      LLVMX86CodeGen

      clangBasic
      clangCodeGen
      clangDriver
      clangFrontend
      clangSerialization
  )
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      LLVM

      clangBasic
      clangCodeGen
      clangDriver
      clangFrontend
      clangSerialization
  )
endif()

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/jit.h
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
if (RAWKIT_JIT_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()