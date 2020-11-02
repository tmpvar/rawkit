add_executable(rawkit-image-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-image-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-image-test
  PRIVATE
    rawkit::image
)

target_include_directories(rawkit-image-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-image-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-image rawkit-image-test)