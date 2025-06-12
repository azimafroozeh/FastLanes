#ifndef DATA_IMAGE_DS_HPP
#define DATA_IMAGE_DS_HPP

#include "fls/std/string.hpp"
#include <array>

namespace fastlanes {
using IMAGE_DS_DATASET_dataset_t = std::array<std::pair<string_view, string_view>, 1>;

class IMAGE_DS {
public:
	static constexpr string_view N1 {FLS_CMAKE_SOURCE_DIR "/data/image/"};

	static constexpr IMAGE_DS_DATASET_dataset_t dataset = {{
	    {"table_2024Q3", N1},
	}};
};
} // namespace fastlanes

#endif // DATA_IMAGE_DS_HPP
