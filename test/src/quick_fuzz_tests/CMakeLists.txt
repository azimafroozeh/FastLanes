add_executable(quick_fuzz_test
        quick_fuzz_test.cpp
)

target_link_libraries(quick_fuzz_test
        PRIVATE
        GTest::gtest_main
        FastLanes
)

fls_enable_sanitizers(quick_fuzz_test)

gtest_discover_tests(quick_fuzz_test
        EXTRA_ARGS --gtest_filter=QuickFuzz.*
        PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        TIMEOUT 120
        DISCOVERY_TIMEOUT 30
)
