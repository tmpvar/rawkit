add_executable(rawkit-hash-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-hash-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-hash-test
  PRIVATE
    rawkit::hash
    pull::stream
)

target_include_directories(rawkit-hash-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-hash-test
  PRIVATE
    cxx_std_14
)


add_test(rawkit-hash rawkit-hash-test)