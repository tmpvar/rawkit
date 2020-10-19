# DEP: doctest
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

add_executable(rawkit-diskwatcher-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-diskwatcher-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-diskwatcher-test PUBLIC
  rawkit::diskwatcher
)

target_include_directories(rawkit-diskwatcher-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-diskwatcher-test
  PRIVATE
    cxx_std_14
)


add_test(rawkit-diskwatcher rawkit-diskwatcher-test)