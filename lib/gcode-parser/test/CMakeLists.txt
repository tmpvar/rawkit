add_executable(gcode-parser-test
  ${CMAKE_CURRENT_LIST_DIR}/gcode-parser-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_include_directories(gcode-parser-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(gcode-parser-test
  PRIVATE
    cxx_std_14
)

target_link_libraries(gcode-parser-test
  PRIVATE
    rawkit::gcode::parser
)

add_test(rawkit-gcode-parser gcode-parser-test)