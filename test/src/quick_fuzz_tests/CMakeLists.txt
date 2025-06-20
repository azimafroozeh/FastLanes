add_executable(quick_fuzz_test
        quick_fuzz_test.cpp
)

target_link_libraries(quick_fuzz_test
        PRIVATE
        gtest_main
        FastLanes
)

fls_enable_sanitizers(quick_fuzz_test)

add_test(
        NAME quick_fuzz_test
        COMMAND quick_fuzz_test
        --gtest_filter=QuickFuzz.*   # only run your fuzz suite
)
set_tests_properties(quick_fuzz_test PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        TIMEOUT 120
)
