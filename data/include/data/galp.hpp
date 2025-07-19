#ifndef DATA_GALP_HPP
#define DATA_GALP_HPP

#include <array>
#include <string>

namespace fastlanes {
using galp_dataset_t = std::array<std::pair<std::string_view, std::string_view>, 2>;

class galp {
public:
	static constexpr std::string_view one_vector {FLS_CMAKE_SOURCE_DIR "/data/generated/galp/one_vector"};
	static constexpr std::string_view single_column {FLS_CMAKE_SOURCE_DIR "/data/generated/galp/single_column"};

	static constexpr galp_dataset_t dataset = {{
	    {"one_vector", one_vector},
	    {"single_column", single_column},

	}};
};
} // namespace fastlanes

#endif // DATA_GALP_HPP
