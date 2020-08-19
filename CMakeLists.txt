cmake_minimum_required(VERSION 3.17)
project(rawkit)

set(DEPS_DIR ${CMAKE_CURRENT_LIST_DIR}/deps)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/SRC)

# DEP: llvm
find_package(LLVM REQUIRED CONFIG
  PATHS ${LLVM_SEARCH_PATHS}
  NO_DEFAULT_PATH
)
find_package(Clang REQUIRED CONFIG
  PATHS  ${LLVM_SEARCH_PATHS}
  NO_DEFAULT_PATH
)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${LLVM_INCLUDE_DIRS}
  include
  include/hot/guest
)

# DEP: Vulkan
if(APPLE)
    set(MOLTENVK_PATH "$ENV{VULKAN_SDK}")
    target_link_directories(rawkit PUBLIC "${MOLTENVK_PATH}/lib")
    set(VULKAN_LIBS
        vulkan
        "-framework IOSurface"
        "-framework Metal"
        "-framework QuartzCore"
    )
    include_directories(${MOLTENVK_PATH}/include)
elseif(WIN32)
    find_package(Vulkan)
    set(VULKAN_LIBS
        ${Vulkan_LIBRARIES}
    )
    include_directories(${Vulkan_INCLUDE_DIRS})
    message(STATUS "vulkan sdk path ${VK_SDK_PATH} ${Vulkan_INCLUDE_DIRS} ${Vulkan_LIBRARIES}")

    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endif()


# DEP: glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/glfw)

# DEP: SPIRV-Headers
# set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
# set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
# add_subdirectory(${DEPS_DIR}/SPIRV-Headers)
# add_definitions(-DSPIRV-Headers_SOURCE_DIR="${DEPS_DIR}/SPIRV-Headers")

# DEP: SPIRV-Tools
# set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
# set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
# add_subdirectory(${DEPS_DIR}/SPIRV-Tools)

# DEP: glslang
# set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
# set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
# set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
# # set(ENABLE_PCH OFF CACHE BOOL "" FORCE)
# set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
# add_subdirectory(${DEPS_DIR}/glslang)

# DEP: shaderc
# set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
# add_subdirectory(${DEPS_DIR}/shaderc)

# DEP: cimgui / imgui
set(IMGUI_SRC
  ${DEPS_DIR}/cimgui/cimgui.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_demo.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_draw.cpp
  ${DEPS_DIR}/cimgui/imgui/imgui_widgets.cpp
  ${DEPS_DIR}/cimgui/imgui/examples/imgui_impl_glfw.cpp
  ${DEPS_DIR}/cimgui/imgui/examples/imgui_impl_vulkan.cpp
)

include_directories(${DEPS_DIR}/cimgui ${DEPS_DIR}/cimgui/imgui)

# DEP: serial
add_subdirectory(${DEPS_DIR}/serial)
include_directories(
  ${DEPS_DIR}/serial/include
  ${DEPS_DIR}/whereami/src
)

add_executable(
  rawkit
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/hot/jitjob.cpp
  ${SRC_DIR}/hot/state.cpp
  ${SRC_DIR}/hot/cimgui.cpp
  ${SRC_DIR}/rawkit/serial.cpp
  ${DEPS_DIR}/whereami/src/whereami.c
  ${IMGUI_SRC}
)


if (WIN32)
  set_property(TARGET rawkit PROPERTY CXX_STANDARD 11)
    set(EXTRA_COMPILE_OPTIONS "/MD")
else()
  set_property(TARGET rawkit PROPERTY CXX_STANDARD 14)
  set(EXTRA_COMPILE_OPTIONS "-Wl,export-dynamic")
endif()

set(LLVM_SEARCH_PATHS
  "E:/llvm/" #tmpvar's pc
  # general homebrew install
  "/usr/local/opt/llvm/lib/cmake/llvm"
  "/usr/local/opt/llvm/lib/cmake/clang"
)


target_compile_options(
  rawkit
  PRIVATE
  -D_CRT_SECURE_NO_DEPRECATE
  -D_CRT_SECURE_NO_WARNINGS
  -D_CRT_NONSTDC_NO_DEPRECATE
  -D_CRT_NONSTDC_NO_WARNINGS
  -D_SCL_SECURE_NO_DEPRECATE
  -D_SCL_SECURE_NO_WARNINGS
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


target_link_libraries(
  rawkit
  glfw
  # grab this from the vulkan sdk
  # shaderc
  serial
  ${VULKAN_LIBS}

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

add_executable(rawkit-test
  ${CMAKE_CURRENT_LIST_DIR}/test/test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/grbl/test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/gcode/test.cpp
)

enable_testing()
add_test(NAME run-tests COMMAND rawkit-test)


file(GLOB RAWKIT_GRBL_GUEST_INCLUDE_FILES "${CMAKE_CURRENT_LIST_DIR}/include/rawkit/grbl/*.h")
list(FILTER RAWKIT_GRBL_GUEST_INCLUDE_FILES EXCLUDE REGEX ".*test.*")
install(
    FILES
        ${RAWKIT_GRBL_GUEST_INCLUDE_FILES}
    DESTINATION include/rawkit/grbl
)

file(GLOB RAWKIT_GRBL_GUEST_INCLUDE_FILES "${CMAKE_CURRENT_LIST_DIR}/include/rawkit/gcode/*.h")
list(FILTER RAWKIT_GRBL_GUEST_INCLUDE_FILES EXCLUDE REGEX ".*test.*")
install(
    FILES
        ${RAWKIT_GRBL_GUEST_INCLUDE_FILES}
    DESTINATION include/rawkit/gcode
)

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
      ${CMAKE_CURRENT_LIST_DIR}/include/stb_sb.h
    DESTINATION include/
)

install(
    TARGETS
        rawkit
    RUNTIME DESTINATION
        bin
)
