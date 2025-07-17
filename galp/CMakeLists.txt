project(GALP LANGUAGES CXX CUDA)

# ────────────────────────────────────────────────
# 0. Global settings
# ────────────────────────────────────────────────
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr") #todo FIX ME

# ────────────────────────────────────────────────
# 1. CUDA Toolkit
# ────────────────────────────────────────────────
find_package(CUDAToolkit REQUIRED)

# ────────────────────────────────────────────────
# 2. nvCOMP (required)
# ────────────────────────────────────────────────
find_package(nvcomp CONFIG REQUIRED)

get_target_property(NVCOMP_VERSION nvcomp::nvcomp VERSION)
get_target_property(NVCOMP_INC_DIRS nvcomp::nvcomp INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(NVCOMP_LIBS nvcomp::nvcomp INTERFACE_LINK_LIBRARIES)

message(STATUS "nvCOMP version ..........: ${NVCOMP_VERSION}")
message(STATUS "nvCOMP include dirs .....: ${NVCOMP_INC_DIRS}")
message(STATUS "nvCOMP libs .............: ${NVCOMP_LIBS}")

# ────────────────────────────────────────────────
# 3. Thrust / CCCL – prefer toolkit copy, fall back to CCCL ≥ 2.3
# ────────────────────────────────────────────────
find_path(THRUST_INCLUDE_DIR
        NAMES thrust/version.h
        thrust/device_uvector.h
        HINTS ${CUDAToolkit_INCLUDE_DIRS})

if (NOT THRUST_INCLUDE_DIR)
    message(FATAL_ERROR "Need Thrust ≥ 2.2 (with device_uvector.h) in CUDA Toolkit")
endif ()

add_library(Thrust::Thrust INTERFACE IMPORTED)
target_include_directories(Thrust::Thrust INTERFACE ${THRUST_INCLUDE_DIR})

# ────────────────────────────────────────────────
# 4. User-tunable options & warnings
# ────────────────────────────────────────────────
option(USE_CLANG_FOR_HOST "Use clang++-14 for all .cpp files" ON)
option(GALP_VERBOSE_BUILD "Print every compile/link command" OFF)
if (GALP_VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

set(COMMON_WARNINGS "")
set(CLANG_ONLY_WARNINGS
        -Wno-c++98-compat-local-type-template-args
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat
        -Wno-padded
        -Wno-float-equal
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-float-conversion
        -Wno-sign-conversion
        -Wno-implicit-int-float-conversion
        -Wno-unused-parameter
        -Wno-unused-variable
)

macro(apply_warnings tgt)
    target_compile_options(${tgt} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${COMMON_WARNINGS}>
            $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:${CLANG_ONLY_WARNINGS}>
            $<$<COMPILE_LANGUAGE:CUDA>:${COMMON_WARNINGS}>)
endmacro()

add_compile_definitions(FLS_GALP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# ────────────────────────────────────────────────
# 5. GALP core static library
# ────────────────────────────────────────────────
set(GALP_CORE_SOURCES
        # alp
        src/alp/alp-bindings.cu
        src/alp/falp.cpp
        # fls
        src/fls/ffor.cpp
        src/fls/pack.cpp
        src/fls/unffor.cpp
        src/fls/unpack.cpp
        # engine
        src/engine/enums.cu
        # nvcomp helpers
        src/nvcomp/benchmark-compressors.cu
        src/nvcomp/nvcomp-compressors.cu
)
add_library(galp_core STATIC ${GALP_CORE_SOURCES})
target_compile_options(galp_core PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-expt-relaxed-constexpr>)
target_include_directories(galp_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CUDAToolkit_INCLUDE_DIRS})
target_link_libraries(galp_core PUBLIC CUDA::cudart nvcomp::nvcomp Thrust::Thrust)
apply_warnings(galp_core)
set_target_properties(galp_core PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# ────────────────────────────────────────────────
# 6. Generated-bindings library
# ────────────────────────────────────────────────
set(GALP_GENERATED_BINDINGS_SOURCES
        src/generated-bindings/alp-double-decompress_column-bindings.cu
        src/generated-bindings/alp-double-query_column-bindings.cu
        src/generated-bindings/alp-double-query_multi_column-bindings.cu
        src/generated-bindings/alp-float-decompress_column-bindings.cu
        src/generated-bindings/alp-float-query_column-bindings.cu
        src/generated-bindings/alp-float-query_multi_column-bindings.cu
        src/generated-bindings/alpextended-double-decompress_column-bindings.cu
        src/generated-bindings/alpextended-double-query_column-bindings.cu
        src/generated-bindings/alpextended-double-query_multi_column-bindings.cu
        src/generated-bindings/alpextended-float-decompress_column-bindings.cu
        src/generated-bindings/alpextended-float-query_column-bindings.cu
        src/generated-bindings/alpextended-float-query_multi_column-bindings.cu
        src/generated-bindings/bp-uint32_t-decompress_column-bindings.cu
        src/generated-bindings/bp-uint32_t-query_column-bindings.cu
        src/generated-bindings/bp-uint64_t-decompress_column-bindings.cu
        src/generated-bindings/bp-uint64_t-query_column-bindings.cu
        src/generated-bindings/ffor-uint32_t-compute_column-bindings.cu
        src/generated-bindings/ffor-uint32_t-decompress_column-bindings.cu
        src/generated-bindings/ffor-uint32_t-query_column-bindings.cu
        src/generated-bindings/ffor-uint32_t-query_multi_column-bindings.cu
        src/generated-bindings/ffor-uint64_t-compute_column-bindings.cu
        src/generated-bindings/ffor-uint64_t-decompress_column-bindings.cu
        src/generated-bindings/ffor-uint64_t-query_column-bindings.cu
        src/generated-bindings/ffor-uint64_t-query_multi_column-bindings.cu
)
add_library(generated-bindings STATIC ${GALP_GENERATED_BINDINGS_SOURCES})
target_compile_options(generated-bindings PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-expt-relaxed-constexpr>)
target_include_directories(generated-bindings PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CUDAToolkit_INCLUDE_DIRS})
target_link_libraries(generated-bindings PUBLIC CUDA::cudart nvcomp::nvcomp Thrust::Thrust)
apply_warnings(generated-bindings)
set_target_properties(generated-bindings PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# ────────────────────────────────────────────────
# 7. Benchmarks
# ────────────────────────────────────────────────
add_executable(compressors-benchmarks src/compressors-benchmarks.cu)
apply_warnings(compressors-benchmarks)
target_link_libraries(compressors-benchmarks PRIVATE galp_core)

add_executable(micro-benchmarks src/micro-benchmarks.cu)
apply_warnings(micro-benchmarks)
target_link_libraries(micro-benchmarks PRIVATE galp_core generated-bindings)

# ────────────────────────────────────────────────
# 8. Tests
# ────────────────────────────────────────────────
add_subdirectory(test)
