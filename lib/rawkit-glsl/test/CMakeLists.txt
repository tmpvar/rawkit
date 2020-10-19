# DEP: doctest
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

add_executable(rawkit-glsl-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-glsl-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-glsl-test
  PRIVATE
    rawkit::glsl
)

target_include_directories(rawkit-glsl-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-glsl-test
  PRIVATE
    cxx_std_14
)


add_test(rawkit-glsl rawkit-glsl-test)