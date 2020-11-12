add_executable(rawkit-vg-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-vg-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-vg-test
  PRIVATE
    rawkit::vg
)

target_include_directories(rawkit-vg-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-vg-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-vg rawkit-vg-test)