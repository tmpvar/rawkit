cmake_minimum_required(VERSION 3.12)
project(rawkit-parser-libs)
set(CMAKE_CXX_STANDARD 11)
include(CTest)


include_directories(
  test/include
  parser/gcode/include
  parser/grbl/include
  ../include/
)

add_executable(
  test-rawkit-libs
  test/test.cpp

  parser/gcode/test.cpp
  parser/grbl/test.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "ADDING COMPILE OPTIONS FOR CLANG")
  target_compile_options(
    test-rawkit-libs PRIVATE
    --coverage
    -g
  )
  target_link_options(
    test-rawkit-libs PRIVATE
    --coverage
  )


elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(
    test-rawkit-libs PRIVATE
    -fprofile-arcs
    -ftest-coverage
    -g
  )

  target_link_options(
    test-rawkit-libs PRIVATE
    -lgcov
    --coverage
  )

endif()

enable_testing()
add_test(NAME rawkit-libs COMMAND test-rawkit-libs)

file(GLOB RAWKIT_GRBL_GUEST_INCLUDE_FILES "${CMAKE_CURRENT_LIST_DIR}/parser/grbl/include/rawkit/grbl/*.h")
install(
    FILES
        ${RAWKIT_GRBL_GUEST_INCLUDE_FILES}
    DESTINATION include/rawkit/grbl
)

file(GLOB RAWKIT_GRBL_GUEST_INCLUDE_FILES "${CMAKE_CURRENT_LIST_DIR}/parser/gcode/include/rawkit/gcode/*.h")
install(
    FILES
        ${RAWKIT_GRBL_GUEST_INCLUDE_FILES}
    DESTINATION include/rawkit/gcode
)
