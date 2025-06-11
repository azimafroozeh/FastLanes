# C++ Example : --------------------------------------------------------------------------------------------------------
add_executable(fastlanes_exmaple ${CMAKE_CURRENT_SOURCE_DIR}/fastlanes.cpp)
target_link_libraries(cpp_example PUBLIC FastLanes)


add_executable(jpeg_to_csv jpeg_decode_to_csv.cpp)
target_link_libraries(jpeg_to_csv PUBLIC FastLanes)
