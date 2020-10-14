cmake_minimum_required(VERSION 3.17)
project(rawkit)

set(DEPS_DIR ${CMAKE_CURRENT_LIST_DIR}/deps)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
  include
  include/hot/guest
)

# DEP: Vulkan
if(APPLE)
    set(MOLTENVK_PATH "$ENV{VULKAN_SDK}")
    link_directories("${MOLTENVK_PATH}/lib")
    set(VULKAN_LIBS
        vulkan
        "-framework IOSurface"
        "-framework Metal"
        "-framework QuartzCore"
    )
    set(RAWKIT_GUEST_VULKAN_INCLUDE_DIR ${MOLTENVK_PATH}/include)

else()
    find_package(Vulkan)
    set(VULKAN_LIBS
        ${Vulkan_LIBRARIES}
    )
    set(RAWKIT_GUEST_VULKAN_INCLUDE_DIR ${Vulkan_INCLUDE_DIRS})
    message(STATUS "vulkan sdk path ${VK_SDK_PATH} ${Vulkan_INCLUDE_DIRS} ${Vulkan_LIBRARIES}")

    # get_cmake_property(_variableNames VARIABLES)
    # list (SORT _variableNames)
    # foreach (_variableName ${_variableNames})
    #     message(STATUS "${_variableName}=${${_variableName}}")
    # endforeach()
endif()

include_directories(${RAWKIT_GUEST_VULKAN_INCLUDE_DIR})

# DEP: glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/glfw)

# DEP: cimgui / imgui
set(IMGUI_SRC
  ${DEPS_DIR}/cimgui/cimgui.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_demo.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_draw.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_widgets.cpp
  ${DEPS_DIR}/cimgui/imgui/examples/imgui_impl_glfw.cpp
  ${SRC_DIR}/imgui/imgui_impl_vulkan.cpp
)

include_directories(
  ${DEPS_DIR}/cimgui
  ${DEPS_DIR}/cimgui/imgui
)

# DEP: serial
add_subdirectory(${DEPS_DIR}/serial)
include_directories(
  ${DEPS_DIR}/serial/include
)

# DEP: tinyfiledialogs
include_directories(
  ${DEPS_DIR}/tinyfiledialogs
)

# DEP: croaring
set(CROARING_SRC
  ${DEPS_DIR}/croaring/roaring/roaring.c
)

include_directories(
  ${DEPS_DIR}/croaring
)

# LIB: pull-stream
set(PULL_STREAM_BUILD_TESTS OFF CACHE INTERNAL "")
set(PULL_STREAM_TEST_COVERAGE ON CACHE INTERNAL "")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pull-stream)
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/lib/pull-stream/include
)

# LIB: gcode-parser
set(RAWKIT_GCODE_PARSER_BUILD_TESTS OFF CACHE INTERNAL "")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/gcode-parser)
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/lib/gcode-parser/include
)

# LIB: grbl-parser
set(RAWKIT_GRBL_PARSER_BUILD_TESTS OFF CACHE INTERNAL "")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/grbl-parser)
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/lib/grbl-parser/include
)

# LIB: rawkit-jit
set(RAWKIT_JIT_BUILD_TESTS OFF CACHE INTERNAL "")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/jit)
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/lib/jit/include
)


add_executable(
  rawkit
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/hot/state.cpp
  ${SRC_DIR}/hot/cimgui.cpp
  ${SRC_DIR}/rawkit/serial.cpp
  ${DEPS_DIR}/tinyfiledialogs/tinyfiledialogs.c
  ${IMGUI_SRC}
  ${CROARING_SRC}
)


if (CMAKE_SYSTEM_NAME STREQUAL Windows)
  set_property(TARGET rawkit PROPERTY CXX_STANDARD 11)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(EXTRA_LINKER_OPTIONS "-export-dynamic")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set_property(TARGET rawkit PROPERTY CXX_STANDARD 14)
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

target_compile_options(
  rawkit
  PRIVATE
  -DUNICODE
  -D_UNICODE
  -D__STDC_CONSTANT_MACROS
  -D__STDC_FORMAT_MACROS
  -D__STDC_LIMIT_MACROS
  # see: https://developercommunity.visualstudio.com/content/problem/93889/error-c2872-byte-ambiguous-symbol.html
  -D_HAS_STD_BYTE=0
  # see: http://www.suodenjoki.dk/us/archive/2010/min-max.htm
  -DNOMINMAX=1

  # TODO: install these headers so we don't have to include them
  -DRAWKIT_CIMGUI_INCLUDE_DIR="-I${DEPS_DIR}/cimgui"
  -DRAWKIT_GUEST_INCLUDE_DIR="-I${CMAKE_CURRENT_LIST_DIR}/include/hot/guest"
  ${EXTRA_COMPILE_OPTIONS}
)

target_link_options(
  rawkit
  PRIVATE
  ${EXTRA_LINKER_OPTIONS}
)

target_link_libraries(
  rawkit
  rawkit-jit

  glfw
  # grab this from the vulkan sdk
  # shaderc
  serial
  ${VULKAN_LIBS}
)

# TODO: install all headers under a rawkit prefix so we don't blow away existing entries in a global install on unix
file(GLOB HOT_GUEST_INCLUDE_FILES "${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/hot/*.h")
install(
    FILES
      ${HOT_GUEST_INCLUDE_FILES}
    DESTINATION include/rawkit/hot
)

install(
    DIRECTORY
      ${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/rawkit/
    DESTINATION include/rawkit
)


file(GLOB SHARED_GUEST_INCLUDE_FILES "${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/*.h")
install(
    FILES
      ${SHARED_GUEST_INCLUDE_FILES}
    DESTINATION include/
)

install(
    FILES
      ${DEPS_DIR}/cimgui/cimgui.h
      ${DEPS_DIR}/cimgui/imgui/imgui.h
      ${DEPS_DIR}/cimgui/imgui/imgui_internal.h
      ${DEPS_DIR}/cimgui/imgui/imconfig.h
      ${DEPS_DIR}/cimgui/imgui/imstb_rectpack.h
      ${DEPS_DIR}/cimgui/imgui/imstb_textedit.h
      ${DEPS_DIR}/cimgui/imgui/imstb_truetype.h
      ${CMAKE_CURRENT_LIST_DIR}/include/stb_sb.h
      ${DEPS_DIR}/tinyfiledialogs/tinyfiledialogs.h
    DESTINATION include/
)

# dirent
if(WIN32)
  install(
      FILES
        ${CMAKE_CURRENT_LIST_DIR}/include/win32_dirent.h
      DESTINATION include
      RENAME dirent.h
  )
endif()

install(
    FILES
      ${DEPS_DIR}/croaring/roaring/roaring.h
    DESTINATION include/roaring
)


# vulkan headers
install(
  DIRECTORY
    ${RAWKIT_GUEST_VULKAN_INCLUDE_DIR}/vulkan/
  DESTINATION
    include/vulkan/
)

install(
    TARGETS
        rawkit
    RUNTIME DESTINATION
        bin
)


# tests
include(CTest)
enable_testing()

set(RAWKIT_TEST_FILES
  ${RAWKIT_GCODE_PARSER_TEST_FILES}
  ${RAWKIT_GRBL_PARSER_TEST_FILES}
  ${PULL_STREAM_TEST_FILES}
  ${RAWKIT_JIT_TEST_FILES}
)

add_executable(rawkit-test
  ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
  ${RAWKIT_TEST_FILES}
)

set_property(TARGET rawkit-test PROPERTY CXX_STANDARD 14)
set_property(TARGET rawkit-test PROPERTY C_STANDARD 11)

source_group("tests" FILES ${RAWKIT_TEST_FILES})

if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set(RAWKIT_TEST_COMPILE_FLAGS
    -D_CRT_SECURE_NO_WARNINGS
  )
else()
  set(COVERAGE_DIR ${PROJECT_BINARY_DIR}/coverage)
  message(STATUS ${COVERAGE_DIR})
  set(RAWKIT_TEST_COMPILE_FLAGS
    -Wall
    -g
    -O0
    --coverage
  )
  target_link_options(rawkit-test PRIVATE
    --coverage
  )
endif()

target_link_libraries(rawkit-test PRIVATE
  pull-stream
  rawkit-jit
)

target_compile_options(rawkit-test PRIVATE
  ${RAWKIT_TEST_COMPILE_FLAGS}
  ${RAWKIT_JIT_EXTRA_COMPILE_FLAGS}
)

target_link_options(rawkit-test PRIVATE
  ${RAWKIT_TEST_COMPILE_FLAGS}
  ${RAWKIT_JIT_EXTRA_LINK_FLAGS}
)

target_include_directories(rawkit-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
  ${LLVM_INCLUDE_DIRS}
)

add_test(rawkit-test rawkit-test)