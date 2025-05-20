cmake_minimum_required(VERSION 3.14)
project(fetch_content_example LANGUAGES CXX)

if (NOT DEFINED FASTLANES_TAG)
    message(FATAL_ERROR "You must define FASTLANES_TAG")
endif ()

include(FetchContent)
FetchContent_Declare(
        fastlanes
        GIT_REPOSITORY https://github.com/azimafroozeh/FastLanes.git
        GIT_TAG ${FASTLANES_TAG}
)
FetchContent_MakeAvailable(fastlanes)

# … rest of your targets …
add_executable(example example.cpp)
target_link_libraries(example PRIVATE fastlanes)
