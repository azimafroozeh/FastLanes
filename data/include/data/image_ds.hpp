#ifndef DATA_IMAGE_DS_HPP
#define DATA_IMAGE_DS_HPP

#include "fls/std/string.hpp"
#include <array>

namespace fastlanes {
using IMAGE_DS_DATASET_dataset_t = std::array<std::pair<string_view, string_view>, 8>;

class IMAGE_DS {
public:
	static constexpr string_view zigzag64 {FLS_CMAKE_SOURCE_DIR "/data/image/zigzag64"};
	static constexpr string_view zigzag_stream {FLS_CMAKE_SOURCE_DIR "/data/image/zigzag_stream"};
	static constexpr string_view raw64 {FLS_CMAKE_SOURCE_DIR "/data/image/raw64"};
	static constexpr string_view raw_stream {FLS_CMAKE_SOURCE_DIR "/data/image/raw_stream"};
	static constexpr string_view zigzag_index_stream {FLS_CMAKE_SOURCE_DIR "/data/image/zigzag_index_stream"};
	static constexpr string_view raw_index {FLS_CMAKE_SOURCE_DIR "/data/image/raw_index"};
	static constexpr string_view raw_nonzero {FLS_CMAKE_SOURCE_DIR "/data/image/raw_nonzero"};
	static constexpr string_view zigzag_nonzero {FLS_CMAKE_SOURCE_DIR "/data/image/zigzag_nonzero"};

	static constexpr IMAGE_DS_DATASET_dataset_t dataset = {{
	    {"zigzag64", zigzag64},
	    {"zigzag_stream", zigzag_stream},
	    {"raw64", raw64},
	    {"raw_stream", raw_stream},
	    {"zigzag_index_stream", zigzag_index_stream},
	    {"raw_index", raw_index},
	    {"raw_nonzero", raw_nonzero},
	    {"zigzag_nonzero", zigzag_nonzero},

	}};
};
} // namespace fastlanes

#endif // DATA_IMAGE_DS_HPP
