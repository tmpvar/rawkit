add_executable(rawkit-texture-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-texture-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-texture-test
  PRIVATE
    rawkit::texture
)

target_include_directories(rawkit-texture-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-texture-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-texture rawkit-texture-test)