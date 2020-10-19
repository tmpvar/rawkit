cmake_minimum_required(VERSION 3.14)
project(rawkit-grbl-parser)


option(RAWKIT_GRBL_PARSER_BUILD_TESTS "Build the pull-stream test programs" ON)

add_library(${PROJECT_NAME} INTERFACE)
add_library(rawkit::grbl::parser ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Install
install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/include/rawkit/grbl/parser.h
  DESTINATION include/rawkit/grbl
)

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  INCLUDES DESTINATION include
)

install(EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE rawkit::
  DESTINATION lib/cmake/grbl
)

# Testing
if (RAWKIT_GRBL_PARSER_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()