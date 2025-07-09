cmake_minimum_required(VERSION 3.24)
project(FastLanesGpu LANGUAGES CXX CUDA)

# ────────────────────────────────────────────────
#  Options
# ────────────────────────────────────────────────
option(USE_CLANG_FOR_HOST "Use clang++-14 for all .cpp files" ON)
option(GALP_ENABLE_NVCOMP "Build nvCOMP codecs" OFF)
option(GALP_VERBOSE_BUILD "Print every compile/link command" OFF)

if (GALP_VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

# ────────────────────────────────────────────────
#  Toolchain paths (adjust if needed)
# ────────────────────────────────────────────────
set(GPU_ARCH "61" CACHE STRING "sm_xy")
set(CUDAToolkit_ROOT "/usr/local/cuda-12.5" CACHE PATH "")

set(CLANG_PATH "/usr/bin/clang++-14" CACHE FILEPATH "")
set(GCC_PATH "/usr/bin/g++-12" CACHE FILEPATH "")

if (GALP_ENABLE_NVCOMP)
    set(NVCOMP_ROOT "/usr/lib/x86_64-linux-gnu/nvcomp/12" CACHE PATH "")
    set(NVCOMP_INCLUDE "/usr/include/nvcomp_12" CACHE PATH "")
endif ()

# ────────────────────────────────────────────────
#  Host compiler
# ────────────────────────────────────────────────
if (USE_CLANG_FOR_HOST)
    set(CMAKE_CXX_COMPILER "${CLANG_PATH}" CACHE FILEPATH "" FORCE)
endif ()

# ────────────────────────────────────────────────
#  CUDA language settings
# ────────────────────────────────────────────────
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# ────────────────────────────────────────────────
#  Include / link dirs
# ────────────────────────────────────────────────
include_directories("${CUDAToolkit_ROOT}/include" "${CMAKE_SOURCE_DIR}")
link_directories("${CUDAToolkit_ROOT}/lib64")

if (GALP_ENABLE_NVCOMP)
    include_directories("${NVCOMP_INCLUDE}")
    link_directories("${NVCOMP_ROOT}")
endif ()

# ────────────────────────────────────────────────
#  C++ warning flags
# ────────────────────────────────────────────────
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(PROJECT_WARNINGS
            -Weverything
            -Wno-c++98-compat-local-type-template-args
            -Wno-c++98-compat-pedantic
            -Wno-c++98-compat
            -Wno-padded
            -Wno-float-equal
            -Wno-global-constructors
            -Wno-exit-time-destructors
            -Wno-float-conversion
            -Wno-sign-conversion
            -Wno-implicit-int-float-conversion)
else ()
    set(PROJECT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow)
endif ()

# ────────────────────────────────────────────────
#  Source groups
# ────────────────────────────────────────────────
file(GLOB_RECURSE FLS_CPP CONFIGURE_DEPENDS src/fls/*.cpp)
file(GLOB_RECURSE ALP_CPP CONFIGURE_DEPENDS src/alp/*.cpp)

# ── Only build this single generated CU TU ────────────────────────────────────
set(GENERATED_CU
        ${CMAKE_SOURCE_DIR}/src/generated-bindings/alp-double-decompress_column-bindings.cu
)

if (GALP_ENABLE_NVCOMP)
    file(GLOB_RECURSE NVCOMP_CU CONFIGURE_DEPENDS src/nvcomp/*.cu)
endif ()

set(ALP_BINDING_CU src/alp/alp-bindings.cu)
set(ENUMS_CU src/engine/enums.cu)

# ────────────────────────────────────────────────
#  NVCC flag presets
# ────────────────────────────────────────────────
set(FULL_NVCC_FLAGS
        --resource-usage -lineinfo --expt-relaxed-constexpr -arch=sm_${GPU_ARCH})
set(LIGHT_NVCC_FLAGS
        --expt-relaxed-constexpr -arch=sm_${GPU_ARCH})

# ────────────────────────────────────────────────
#  Code-generation helper
# ────────────────────────────────────────────────
add_custom_target(generate
        COMMAND ${CMAKE_COMMAND} -E echo "Running code generators…"
        COMMAND ${CMAKE_SOURCE_DIR}/code-generators/generate-multicolumn-kernels.py
        COMMAND ${CMAKE_SOURCE_DIR}/code-generators/generate-kernel-bindings.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# ────────────────────────────────────────────────
#  Libraries
# ────────────────────────────────────────────────
add_library(core STATIC ${FLS_CPP} ${ALP_CPP})
target_compile_features(core PRIVATE cxx_std_17)
target_compile_options(core PRIVATE -O3 ${PROJECT_WARNINGS})
add_dependencies(core generate)

add_library(generated STATIC ${GENERATED_CU})
set_target_properties(generated PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(generated PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:${LIGHT_NVCC_FLAGS}>)
add_dependencies(generated generate)

add_library(enums STATIC ${ENUMS_CU})
set_target_properties(enums PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(enums PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:${LIGHT_NVCC_FLAGS}>)
add_dependencies(enums generate)

add_library(alp_bindings STATIC ${ALP_BINDING_CU})
set_target_properties(alp_bindings PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_compile_options(alp_bindings PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-ccbin=${CLANG_PATH};${FULL_NVCC_FLAGS}>)
add_dependencies(alp_bindings generate)

if (GALP_ENABLE_NVCOMP)
    add_library(nvcomp_lib STATIC ${NVCOMP_CU})
    set_target_properties(nvcomp_lib PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    target_compile_options(nvcomp_lib PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:-ccbin=${GCC_PATH};${LIGHT_NVCC_FLAGS}>)
    add_dependencies(nvcomp_lib generate)
endif ()

# ────────────────────────────────────────────────
#  Executables
# ────────────────────────────────────────────────
add_executable(micro-benchmarks src/micro-benchmarks.cu)
target_compile_options(micro-benchmarks PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:${LIGHT_NVCC_FLAGS}>)
target_link_libraries(micro-benchmarks
        PRIVATE alp_bindings generated core enums
        cuda curand)

add_executable(compressors-benchmarks src/compressors-benchmarks.cu)
target_compile_options(compressors-benchmarks PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:${FULL_NVCC_FLAGS}>)

if (GALP_ENABLE_NVCOMP)
    target_link_libraries(compressors-benchmarks
            PRIVATE core generated nvcomp_lib alp_bindings enums
            nvcomp cuda curand)
else ()
    target_link_libraries(compressors-benchmarks
            PRIVATE core generated alp_bindings enums
            cuda curand)
endif ()

# ────────────────────────────────────────────────
#  Helper targets
# ────────────────────────────────────────────────
add_custom_target(test
        COMMAND ${CMAKE_SOURCE_DIR}/test-scripts/test-fls.sh
        COMMAND ${CMAKE_SOURCE_DIR}/test-scripts/test-alp.sh
        COMMAND ${CMAKE_SOURCE_DIR}/test-scripts/test-compressors.sh
        COMMAND ${CMAKE_SOURCE_DIR}/test-scripts/test-multi-column.sh)

add_custom_target(benchmark-all
        COMMAND sudo ${CMAKE_SOURCE_DIR}/benchmark-scripts/run-benchmarks.py all benchmark-results/output/)

add_custom_target(benchmark-compressors
        COMMAND sudo ${CMAKE_SOURCE_DIR}/benchmark-scripts/run-benchmarks.py compressors benchmark-results/output/)
