add_executable(fastlanes_cli main.cpp)
target_link_libraries(fastlanes_cli PUBLIC FastLanes)

if (FLS_ENABLE_INSTALL)
    install(TARGETS
            fastlanes_cli RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()