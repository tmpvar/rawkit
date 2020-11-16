add_executable(rawkit-window-test
  ${CMAKE_CURRENT_LIST_DIR}/rawkit-window-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_link_libraries(rawkit-window-test
  PRIVATE
    rawkit::window
)

target_include_directories(rawkit-window-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

target_compile_features(rawkit-window-test
  PRIVATE
    cxx_std_14
)

add_test(rawkit-window rawkit-window-test)