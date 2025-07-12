add_executable(run_tests
        thrust_memcpy_test.cu
        thrust_core_test.cu
)

target_link_libraries(run_tests PRIVATE
        galp_core
        generated-bindings
        GTest::gtest_main
)

apply_warnings(run_tests)

add_test(NAME GALPAllTests COMMAND run_tests)
