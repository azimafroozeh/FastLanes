add_executable(quick_fuzz_test
        quick_fuzz_test.cpp
)

target_link_libraries(quick_fuzz_test
        PRIVATE
        gtest_main
        FastLanes
)

fls_enable_sanitizers(quick_fuzz_test)

gtest_discover_tests(
        quick_fuzz_test
        DISCOVERY_MODE POST_BUILD
        DISCOVERY_TIMEOUT 120
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
