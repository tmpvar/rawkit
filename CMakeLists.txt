cmake_minimum_required(VERSION 3.17)
project(rawkit)

set(DEPS_DIR ${CMAKE_CURRENT_LIST_DIR}/deps)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
  include
  include/hot/guest
)

# DEP: cimgui / imgui
set(IMGUI_SRC
  ${DEPS_DIR}/cimgui/cimgui.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_demo.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_draw.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_widgets.cpp
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

# DEP: flags
include_directories(
  ${DEPS_DIR}/flags/include
)

# DEP: croaring
set(CROARING_SRC
  ${DEPS_DIR}/croaring/roaring/roaring.c
)

include_directories(
  ${DEPS_DIR}/croaring
)

include(CTest)
enable_testing()

# LIB: lz4
if (NOT TARGET rawkit::lz4)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/lz4)
endif()

# LIB: hidapi
if (NOT TARGET hidapi::hidapi)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/deps/hidapi)
  # fix for windows
  include_directories(${DEPS_DIR}/hidapi/hidapi-8741697)
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
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-jit)

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

# LIB: rawkit-vg
if (NOT TARGET rawkit::vg)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-vg)
endif()

# LIB: rawkit-window
if (NOT TARGET rawkit::window)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-window)
endif()

# LIB: rawkit-worker
if (NOT TARGET rawkit::worker)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/rawkit-worker)
endif()

add_executable(
  rawkit
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/hot/state.cpp
  ${SRC_DIR}/hot/cimgui.cpp
  ${SRC_DIR}/rawkit/serial.cpp

  ${SRC_DIR}/imgui/imgui_impl_glfw.cpp
  ${SRC_DIR}/imgui/imgui_impl_vulkan.cpp
  ${DEPS_DIR}/tinyfiledialogs/tinyfiledialogs.c
  ${IMGUI_SRC}
  ${CROARING_SRC}
)

target_compile_features(rawkit
  PRIVATE
    cxx_std_17
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

target_link_options(${PROJECT_NAME}
  PRIVATE
    ${EXTRA_LINKER_OPTIONS}
)

target_compile_features(${PROJECT_NAME}
  PRIVATE
    cxx_std_14
)

target_link_libraries(
  rawkit
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

  rawkit::vg
  rawkit::window
  rawkit::vulkan

  rawkit::worker

  rawkit::lz4
  hidapi::hidapi

  serial
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/hot
  DESTINATION include/rawkit
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/rawkit/
  DESTINATION include/rawkit
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/cglm
  DESTINATION include
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/include/hot/guest/glm
    ${CMAKE_CURRENT_LIST_DIR}/include/ghc
    # libmorton
    ${CMAKE_CURRENT_LIST_DIR}/include/morton
  DESTINATION include
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

install(
  TARGETS
    rawkit
  RUNTIME DESTINATION
    bin
)
