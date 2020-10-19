# DEP: doctest
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

add_executable(rawkit-file-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-file-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-file-test PUBLIC
  rawkit::file
)

target_include_directories(rawkit-file-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-file-test
  PRIVATE
    cxx_std_14
)


add_test(rawkit-file rawkit-file-test)