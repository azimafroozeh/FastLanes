add_executable(compressors-benchmarks compressors-benchmarks.cu)
apply_warnings(compressors-benchmarks)
target_link_libraries(compressors-benchmarks PRIVATE galp_core)

add_executable(micro-benchmarks micro-benchmarks.cu)
apply_warnings(micro-benchmarks)
target_link_libraries(micro-benchmarks PRIVATE galp_core generated-bindings)
