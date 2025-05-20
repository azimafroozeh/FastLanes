cmake_minimum_required(VERSION 3.22)
project(fetch_content_example LANGUAGES CXX)

# — allow overriding which branch/tag/commit of fastlanes to fetch —
set(FASTLANES_GIT_TAG "main" CACHE STRING
        "Branch, tag, or commit SHA to fetch for fastlanes")

# If running under CI, pick up its SHA (GitLab CI uses CI_COMMIT_SHA;
# GitHub Actions provides GITHUB_SHA)
if (DEFINED ENV{CI_COMMIT_SHA})
    message(STATUS "Overriding FastLanes tag to CI_COMMIT_SHA=$ENV{CI_COMMIT_SHA}")
    set(FASTLANES_GIT_TAG $ENV{CI_COMMIT_SHA} CACHE STRING "" FORCE)
elseif (DEFINED ENV{GITHUB_SHA})
    message(STATUS "Overriding FastLanes tag to GITHUB_SHA=$ENV{GITHUB_SHA}")
    set(FASTLANES_GIT_TAG $ENV{GITHUB_SHA} CACHE STRING "" FORCE)
endif ()

include(FetchContent)

FetchContent_Declare(
        fastlanes
        GIT_REPOSITORY https://github.com/cwida/FastLanes.git
        GIT_TAG ${FASTLANES_GIT_TAG}
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(fastlanes)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE fastlanes)
