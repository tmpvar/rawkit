add_executable(rawkit-jit-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-jit-api-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-jit-internal-call-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-jit-internal-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

# DEP: doctest
target_include_directories(rawkit-jit-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

set_property(TARGET rawkit-jit-test PROPERTY CXX_STANDARD 14)
set_property(TARGET rawkit-jit-test PROPERTY C_STANDARD 11)

target_link_libraries(rawkit-jit-test
  PRIVATE
    rawkit::jit
)

target_compile_features(rawkit-jit-test
  PRIVATE
    cxx_std_14
)

target_include_directories(rawkit-jit-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

add_test(rawkit-jit rawkit-jit-test)