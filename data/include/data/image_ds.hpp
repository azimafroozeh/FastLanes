#ifndef DATA_IMAGE_DS_HPP
#define DATA_IMAGE_DS_HPP

#include "fls/std/string.hpp"
#include <array>

namespace fastlanes {
using IMAGE_DS_DATASET_dataset_t = std::array<std::pair<string_view, string_view>, 2>;

class IMAGE_DS {
public:
	static constexpr string_view zigzag64 {FLS_CMAKE_SOURCE_DIR "/data/image/zigzag64"};
	static constexpr string_view zigzag_stream {FLS_CMAKE_SOURCE_DIR "/data/image/zigzag_stream"};

	static constexpr IMAGE_DS_DATASET_dataset_t dataset = {{
	    {"zigzag64", zigzag64},
	    {"zigzag_stream", zigzag_stream},
	}};
};
} // namespace fastlanes

#endif // DATA_IMAGE_DS_HPP
