add_executable(rawkit-shader-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-shader-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-shader-test
  PRIVATE
    rawkit::shader
)

target_include_directories(rawkit-shader-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-shader-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-shader rawkit-shader-test)