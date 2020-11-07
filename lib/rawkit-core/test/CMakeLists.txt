# DEP: doctest
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

add_executable(rawkit-core-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-core-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-core-test PUBLIC
  rawkit::core
)

target_include_directories(rawkit-core-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-core-test
  PRIVATE
    cxx_std_14
)


add_test(rawkit-core rawkit-core-test)