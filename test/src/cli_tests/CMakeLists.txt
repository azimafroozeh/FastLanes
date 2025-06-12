# test/src/cli_tests/CMakeLists.txt

# 1) Test that --help prints usage
add_test(NAME cli_help
        COMMAND $<TARGET_FILE:fastlanes_cli> --help)
set_tests_properties(cli_help
        PROPERTIES PASS_REGULAR_EXPRESSION "fls <command> \\[options\\] <file> …")

# 2) Test that an unknown command falls through
add_test(NAME cli_unknown
        COMMAND $<TARGET_FILE:fastlanes_cli> foobar)
set_tests_properties(cli_unknown
        PROPERTIES PASS_REGULAR_EXPRESSION "Unknown subcommand: foobar")

# 3) Stub test for a recognized-but-not-implemented command
add_test(NAME cli_meta_stub
        COMMAND $<TARGET_FILE:fastlanes_cli> meta sample.parquet)
set_tests_properties(cli_meta_stub
        PROPERTIES PASS_REGULAR_EXPRESSION "\\[fls\\] 'meta' is recognised but not implemented yet\\.")

# 4) You can add more, e.g. head stub, cat stub, etc.
add_test(NAME cli_cat_stub
        COMMAND $<TARGET_FILE:fastlanes_cli> cat sample.parquet)
set_tests_properties(cli_cat_stub
        PROPERTIES PASS_REGULAR_EXPRESSION "\\[fls\\] 'cat' is recognised but not implemented yet\\.")
