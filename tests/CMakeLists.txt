add_executable(micoring_test_all test_all.c)
target_compile_features(micoring_test_all PRIVATE c_std_99)
target_compile_definitions(micoring_test_all PRIVATE MRING_TEST=1)
target_link_libraries(micoring_test_all PRIVATE micoring)

add_executable(micoring_test_harness_selftest test_harness_selftest.c)
target_compile_features(micoring_test_harness_selftest PRIVATE c_std_99)

add_subdirectory(consumers/add_subdirectory_consumer)
add_subdirectory(consumers/cpp_consumer)

if(BUILD_TESTING)
    add_test(NAME micoring_runtime COMMAND micoring_test_all)
    add_test(NAME micoring_harness_selftest COMMAND micoring_test_harness_selftest)
endif()
