cmake_minimum_required(VERSION 3.14)
project(rawkit-vulkan)

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
endif()

if(NOT TARGET rawkit::vulkan)
  add_library(${PROJECT_NAME} INTERFACE)
  add_library(rawkit::vulkan ALIAS ${PROJECT_NAME})

  target_link_libraries(${PROJECT_NAME}
    INTERFACE
      ${VULKAN_LIBS}
  )

  target_include_directories(${PROJECT_NAME}
    INTERFACE
      $<BUILD_INTERFACE:${RAWKIT_GUEST_VULKAN_INCLUDE_DIR}>
      $<INSTALL_INTERFACE:include>
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

  install(
    DIRECTORY
      ${RAWKIT_GUEST_VULKAN_INCLUDE_DIR}/vulkan
    DESTINATION
      include
  )

endif()