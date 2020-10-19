add_executable(pull-stream-test
  ${CMAKE_CURRENT_LIST_DIR}/ps-core-handle-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-core-util-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-io-file-sink-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-io-file-source-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-io-tcp-duplex-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-io-tcp-server-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-collector-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-counter-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-hex-printer-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-multiplier-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-nooper-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-reverser-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-single-value-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-splitter-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-taker-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ps-sync-user-value-test.cpp
  ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)

target_compile_features(pull-stream-test
  PRIVATE
    cxx_std_14
)

target_link_libraries(pull-stream-test
  PRIVATE
    pull::stream
)

target_include_directories(pull-stream-test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/deps/doctest
)

add_test(NAME pull-stream COMMAND pull-stream-test)