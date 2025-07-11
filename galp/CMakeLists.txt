cmake_minimum_required(VERSION 3.18)
project(GALP LANGUAGES CXX CUDA)

# ────────────────────────────────────────────────
# Find CUDA toolkit (for CUDA::cudart target)
# ────────────────────────────────────────────────
find_package(CUDAToolkit REQUIRED)

# ────────────────────────────────────────────────
# Options
# ────────────────────────────────────────────────
option(USE_CLANG_FOR_HOST "Use clang++-14 for all .cpp files" ON)
option(GALP_ENABLE_NVCOMP "Build nvCOMP codecs" OFF)
option(GALP_VERBOSE_BUILD "Print every compile/link command" OFF)

if (GALP_VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

if (GALP_ENABLE_NVCOMP)
    set(NVCOMP_ROOT "/usr/lib/x86_64-linux-gnu/nvcomp/12" CACHE PATH "")
    set(NVCOMP_INCLUDE "/usr/include/nvcomp_12" CACHE PATH "")
    include_directories(${NVCOMP_INCLUDE})
    link_directories(${NVCOMP_ROOT})
endif ()

# ────────────────────────────────────────────────
# Warning flags and standards
# ────────────────────────────────────────────────
## Warnings common to both languages
#set(COMMON_WARNINGS
#        -Wall
#        -Wextra
#        -Wshadow
#)

## Extra warnings for C++ (strict ISO) and silence the GNU line‑marker notice
#set(CXX_WARNINGS
#        ${COMMON_WARNINGS}
#        -Wpedantic
#        -Wno-gnu-line-marker
#)
#
## Extra warnings for CUDA – forward host‑only suppressions via -Xcompiler
#set(CUDA_WARNINGS
#        ${COMMON_WARNINGS}
#        -Xcompiler=-Wno-pedantic
#        -Xcompiler=-Wno-gnu-line-marker
#)


# Clang‑only clean‑up warnings (unchanged)
set(CLANG_ONLY_WARNINGS
        #-Weverything                      # uncomment if you want absolutely everything
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

set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Helper macro to apply the correct warnings per language
macro(apply_warnings target)
    target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${CXX_WARNINGS}>
            $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:${CLANG_ONLY_WARNINGS}>
            $<$<COMPILE_LANGUAGE:CUDA>:${CUDA_WARNINGS}>
    )
endmacro()

# ────────────────────────────────────────────────
# Core GALP static library (explicit source list)
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

        # generated-bindings
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

        # nvcomp (optional helpers)
        # Uncomment if GALP_ENABLE_NVCOMP is ON
        # src/nvcomp/benchmark-compressors.cu
        # src/nvcomp/nvcomp-compressors.cu
)

add_library(galp_core STATIC ${GALP_CORE_SOURCES})

# TODO: remove --expt-relaxed-constexpr and instead mark `set_first_n_bits`
#       as __host__ __device__ constexpr in flsgpu/fls.cuh

target_compile_options(galp_core PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-expt-relaxed-constexpr>
)

if (CUDAToolkit_INCLUDE_DIRECTORIES)
    target_include_directories(galp_core PUBLIC ${CUDAToolkit_INCLUDE_DIRECTORIES})
endif ()

# Public headers
target_include_directories(galp_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

apply_warnings(galp_core)
set_target_properties(galp_core PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

target_link_libraries(galp_core PUBLIC CUDA::cudart)
if (GALP_ENABLE_NVCOMP)
    target_link_libraries(galp_core PUBLIC nvcomp_static)
endif ()

# ────────────────────────────────────────────────
# compressors‑benchmarks executable
# ────────────────────────────────────────────────
add_executable(compressors-benchmarks src/compressors-benchmarks.cu)
apply_warnings(compressors-benchmarks)

target_link_libraries(compressors-benchmarks PRIVATE galp_core)
target_include_directories(compressors-benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ────────────────────────────────────────────────
# micro‑benchmarks executable
# ────────────────────────────────────────────────
add_executable(micro-benchmarks src/micro-benchmarks.cu)
apply_warnings(micro-benchmarks)

target_link_libraries(micro-benchmarks PRIVATE galp_core)
target_include_directories(micro-benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
