cmake_minimum_required(VERSION 3.18)
project(GALP LANGUAGES CXX CUDA)

# ────────────────────────────────────────────────
# CUDA Toolkit (gives us CUDA::cudart, include paths, etc.)
# ────────────────────────────────────────────────
find_package(CUDAToolkit REQUIRED)              # aborts if CUDA not found

# ────────────────────────────────────────────────
# nvCOMP – system package is mandatory
# ────────────────────────────────────────────────
find_package(nvcomp CONFIG REQUIRED)            # aborts if missing

# Pretty log
get_target_property(NVCOMP_VERSION nvcomp::nvcomp VERSION)
get_target_property(NVCOMP_INC_DIRS nvcomp::nvcomp INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(NVCOMP_LIBS nvcomp::nvcomp INTERFACE_LINK_LIBRARIES)
message(STATUS "nvCOMP version ..........: ${NVCOMP_VERSION}")
message(STATUS "nvCOMP include dirs .....: ${NVCOMP_INC_DIRS}")
message(STATUS "nvCOMP libs .............: ${NVCOMP_LIBS}")

# ────────────────────────────────────────────────
# Thrust – try CUDA Toolkit first, otherwise fetch from GitHub
# ────────────────────────────────────────────────
find_path(THRUST_INCLUDE_DIR
        NAMES thrust/version.h
        HINTS ${CUDAToolkit_INCLUDE_DIRS} /usr/include
)
if (THRUST_INCLUDE_DIR)
    message(STATUS "Thrust headers found at ${THRUST_INCLUDE_DIR}")
    add_library(Thrust::Thrust INTERFACE IMPORTED)
    target_include_directories(Thrust::Thrust INTERFACE ${THRUST_INCLUDE_DIR})
else ()
    message(STATUS "Thrust not found in CUDA Toolkit — fetching from GitHub")
    include(FetchContent)
    FetchContent_Declare(
            thrust
            GIT_REPOSITORY https://github.com/NVIDIA/thrust.git
            GIT_TAG v2.0.1                # pick a release you like
    )
    FetchContent_MakeAvailable(thrust)        # exports Thrust::Thrust
endif ()

# ────────────────────────────────────────────────
# User-tunable options
# ────────────────────────────────────────────────
option(USE_CLANG_FOR_HOST "Use clang++-14 for all .cpp files" ON)
option(GALP_VERBOSE_BUILD "Print every compile/link command" OFF)

if (GALP_VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

# ────────────────────────────────────────────────
# Warning flags and language standards
# ────────────────────────────────────────────────
set(COMMON_WARNINGS "")
set(CXX_WARNINGS "${COMMON_WARNINGS}")
set(CUDA_WARNINGS "${COMMON_WARNINGS}")

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

set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

macro(apply_warnings target)
    target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${CXX_WARNINGS}>
            $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:${CLANG_ONLY_WARNINGS}>
            $<$<COMPILE_LANGUAGE:CUDA>:${CUDA_WARNINGS}>
    )
endmacro()

# ────────────────────────────────────────────────
# Core GALP static library
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
        # generated-bindings (trimmed for brevity)
        src/generated-bindings/alp-double-decompress_column-bindings.cu
        #        src/generated-bindings/alp-double-query_column-bindings.cu
        #        src/generated-bindings/alp-double-query_multi_column-bindings.cu
        #        src/generated-bindings/alp-float-decompress_column-bindings.cu
        #        src/generated-bindings/alp-float-query_column-bindings.cu
        #        src/generated-bindings/alp-float-query_multi_column-bindings.cu
        #        src/generated-bindings/alpextended-double-decompress_column-bindings.cu
        #        src/generated-bindings/alpextended-double-query_column-bindings.cu
        #        src/generated-bindings/alpextended-double-query_multi_column-bindings.cu
        #        src/generated-bindings/alpextended-float-decompress_column-bindings.cu
        #        src/generated-bindings/alpextended-float-query_column-bindings.cu
        #        src/generated-bindings/alpextended-float-query_multi_column-bindings.cu
        #        src/generated-bindings/bp-uint32_t-decompress_column-bindings.cu
        #        src/generated-bindings/bp-uint32_t-query_column-bindings.cu
        #        src/generated-bindings/bp-uint64_t-decompress_column-bindings.cu
        #        src/generated-bindings/bp-uint64_t-query_column-bindings.cu
        #        src/generated-bindings/ffor-uint32_t-compute_column-bindings.cu
        #        src/generated-bindings/ffor-uint32_t-decompress_column-bindings.cu
        #        src/generated-bindings/ffor-uint32_t-query_column-bindings.cu
        #        src/generated-bindings/ffor-uint32_t-query_multi_column-bindings.cu
        #        src/generated-bindings/ffor-uint64_t-compute_column-bindings.cu
        #        src/generated-bindings/ffor-uint64_t-decompress_column-bindings.cu
        #        src/generated-bindings/ffor-uint64_t-query_column-bindings.cu
        #        src/generated-bindings/ffor-uint64_t-query_multi_column-bindings.cu

        # … add the rest of your generated sources …
        # nvcomp helpers
        src/nvcomp/benchmark-compressors.cu
        src/nvcomp/nvcomp-compressors.cu         # uncomment if you need it
)

add_library(galp_core STATIC ${GALP_CORE_SOURCES})

# TODO: remove --expt-relaxed-constexpr and instead mark `set_first_n_bits`
#       as __host__ __device__ constexpr in flsgpu/fls.cuh

target_compile_options(galp_core PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-expt-relaxed-constexpr>
)

target_include_directories(galp_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CUDA Toolkit include dirs (if provided)
if (CUDAToolkit_INCLUDE_DIRS)
    target_include_directories(galp_core PUBLIC ${CUDAToolkit_INCLUDE_DIRS})
endif ()

# Link everything we need: cudart, nvcomp, Thrust headers
target_link_libraries(galp_core
        PUBLIC CUDA::cudart
        nvcomp::nvcomp
        Thrust::Thrust
)

apply_warnings(galp_core)
set_target_properties(galp_core PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# ────────────────────────────────────────────────
# Compressors benchmarks executable
# ────────────────────────────────────────────────
add_executable(compressors-benchmarks src/compressors-benchmarks.cu)
apply_warnings(compressors-benchmarks)
target_link_libraries(compressors-benchmarks PRIVATE galp_core)

# ────────────────────────────────────────────────
# Micro-benchmarks executable
# ────────────────────────────────────────────────
add_executable(micro-benchmarks src/micro-benchmarks.cu)
apply_warnings(micro-benchmarks)
target_link_libraries(micro-benchmarks PRIVATE galp_core)
