add_executable(grbl-parser-test
  ${CMAKE_CURRENT_LIST_DIR}/grbl-parser-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_include_directories(grbl-parser-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(grbl-parser-test
  PRIVATE
    cxx_std_14
)

target_link_libraries(grbl-parser-test
  PRIVATE
    rawkit::grbl::parser
)

add_test(rawkit-grbl-parser grbl-parser-test)