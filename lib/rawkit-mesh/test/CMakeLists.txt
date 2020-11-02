add_executable(rawkit-mesh-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-mesh-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-mesh-test
  PRIVATE
    rawkit::mesh
)

target_include_directories(rawkit-mesh-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-mesh-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-mesh rawkit-mesh-test -d 1)