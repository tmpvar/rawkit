cmake_minimum_required(VERSION 3.17)
project(rawkit)

set(DEPS_DIR ${CMAKE_CURRENT_LIST_DIR}/deps)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/SRC)

# DEP: glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/glfw)

# DEP: SPIRV-Headers
add_subdirectory(${DEPS_DIR}/SPIRV-Headers)

# DEP: SPIRV-Tools
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/SPIRV-Tools)

# DEP: glslang
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
# set(ENABLE_PCH OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/glslang)

# DEP: shaderc
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory(${DEPS_DIR}/shaderc)

# DEP: imgui
set(IMGUI_SRC
  ${DEPS_DIR}/imgui/imgui.cpp
  ${DEPS_DIR}/imgui/imgui_demo.cpp
  ${DEPS_DIR}/imgui/imgui_draw.cpp
  ${DEPS_DIR}/imgui/imgui_widgets.cpp
  ${DEPS_DIR}/imgui/examples/imgui_impl_glfw.cpp
  ${DEPS_DIR}/imgui/examples/imgui_impl_vulkan.cpp
)

include_directories(${DEPS_DIR}/imgui ${DEPS_DIR})

add_executable(
  rawkit
  ${SRC_DIR}/main.cpp
  ${IMGUI_SRC}
)


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
    message(STATUS "vulkan sdk path ${VK_SDK_PATH}")
endif()

target_link_libraries(
  rawkit
  glfw
  shaderc
  ${VULKAN_LIBS}
)
