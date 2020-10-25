cmake_minimum_required(VERSION 3.14)
project(rawkit-vulkan)

if(NOT TARGET rawkit::vulkan)
  add_library(${PROJECT_NAME} INTERFACE)
  add_library(rawkit::vulkan ALIAS ${PROJECT_NAME})

  if(APPLE)
      set(MOLTENVK_PATH "$ENV{VULKAN_SDK}")
      target_link_directories(${PROJECT_NAME}
        INTERFACE
          ${MOLTENVK_PATH}/lib
      )

      target_link_libraries(${PROJECT_NAME}
        INTERFACE
          vulkan
          "-framework IOSurface"
          "-framework Metal"
          "-framework QuartzCore"
      )
message(STATUS "molten include dirs: ${MOLTENVK_PATH}/include")
      target_include_directories(${PROJECT_NAME}
        INTERFACE
          ${MOLTENVK_PATH}/include
      )

      install(
        DIRECTORY
          ${MOLTENVK_PATH}/include/vulkan
        DESTINATION
          include
      )
  else()
      find_package(Vulkan)
      target_link_libraries(${PROJECT_NAME}
        INTERFACE
          ${Vulkan_LIBRARIES}
      )

      target_include_directories(${PROJECT_NAME}
        INTERFACE
          $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIRS}>
          $<INSTALL_INTERFACE:include>
      )

      install(
        DIRECTORY
          ${Vulkan_INCLUDE_DIRS}/vulkan
        DESTINATION
          include
      )
  endif()


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

endif()