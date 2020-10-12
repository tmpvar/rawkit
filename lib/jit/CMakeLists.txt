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


add_library(rawkit-jit
  src/api.cpp
  src/job.cpp
  src/runnable.cpp
)

target_include_directories(rawkit-jit PUBLIC
  ${LLVM_INCLUDE_DIRS}
)

target_include_directories(rawkit-jit PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/deps/whereami/src
)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
  set_property(TARGET rawkit-jit PROPERTY CXX_STANDARD 11)
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
  )

endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(EXTRA_LINKER_OPTIONS "-export-dynamic")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set_property(TARGET rawkit-jit PROPERTY CXX_STANDARD 14)
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

target_link_options(rawkit-jit PRIVATE
  ${EXTRA_LINKER_OPTIONS}
)

if (RAWKIT_JIT_TEST_COVERAGE)
  target_compile_options(pull-stream PUBLIC
    -g
    -O0
    --coverage
    -ftest-coverage
    -fprofile-arcs
    -fno-default-inline
    -fno-inline
  )
endif()

target_link_libraries(rawkit-jit PUBLIC
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


# Testing
file(GLOB RAWKIT_JIT_TEST_FILES
  "${CMAKE_CURRENT_LIST_DIR}/test/*-test.cpp"
)
set(RAWKIT_JIT_TEST_FILES ${RAWKIT_JIT_TEST_FILES} CACHE INTERNAL "")
set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS} CACHE INTERNAL "")

set(RAWKIT_JIT_EXTRA_COMPILE_FLAGS ${EXTRA_COMPILE_OPTIONS} CACHE INTERNAL "")
set(RAWKIT_JIT_EXTRA_LINK_FLAGS ${EXTRA_LINK_OPTIONS} CACHE INTERNAL "")

if (RAWKIT_JIT_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_executable(rawkit-jit-test
    ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
    ${RAWKIT_JIT_TEST_FILES}
  )

  # DEP: doctest
  target_include_directories(rawkit-jit-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  set_property(TARGET rawkit-jit-test PROPERTY CXX_STANDARD 17)
  set_property(TARGET rawkit-jit-test PROPERTY C_STANDARD 11)


  source_group("tests" FILES ${RAWKIT_JIT_TEST_FILES})

  target_link_libraries(rawkit-jit-test PRIVATE
    rawkit-jit
  )

  target_include_directories(rawkit-jit-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  )

  add_test(rawkit-jit rawkit-jit-test)
endif()