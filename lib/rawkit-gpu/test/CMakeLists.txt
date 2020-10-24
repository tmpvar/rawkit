add_executable(rawkit-gpu-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-gpu-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-gpu-test
  PRIVATE
    rawkit::gpu
)

target_include_directories(rawkit-gpu-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-gpu-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-gpu rawkit-gpu-test)