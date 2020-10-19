add_executable(rawkit-hot-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-hot-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-hot-test
  PRIVATE
    rawkit::hot
)

target_include_directories(rawkit-hot-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-hot-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-hot rawkit-hot-test)