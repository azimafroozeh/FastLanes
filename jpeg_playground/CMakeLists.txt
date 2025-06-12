include(ExternalProject)

# 1) Download & build libjpeg-turbo as an ExternalProject
ExternalProject_Add(libjpeg_turbo_ep
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG main
        UPDATE_COMMAND ""  # disable re-clone on re-configure
        PREFIX ${CMAKE_BINARY_DIR}/third_party/libjpeg_turbo
        INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/install/libjpeg_turbo
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DWITH_JPEG8=ON
        -DBUILD_TESTS=OFF
)

# 2) Point find_package at that staging area
list(APPEND CMAKE_PREFIX_PATH
        ${CMAKE_BINARY_DIR}/third_party/install/libjpeg_turbo
)

# 3) Now find the installed libjpeg-turbo via its CMake config
find_package(JPEG REQUIRED)

# 4) Your executables
add_executable(jpeg_compress jpeg_analyze.cpp)
target_include_directories(jpeg_compress PUBLIC include)

# 5) Ensure they all wait for libjpeg-turbo to build & install,
#    then link against FastLanes and the JPEG::JPEG target
foreach (target IN ITEMS jpeg_compress)
    add_dependencies(${target} libjpeg_turbo_ep)
    target_link_libraries(${target}
            PUBLIC
            FastLanes
            JPEG::JPEG
    )
endforeach ()
