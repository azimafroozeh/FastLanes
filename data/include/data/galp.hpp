#ifndef DATA_GALP_HPP
#define DATA_GALP_HPP

#include <array>
#include <string_view>

namespace fastlanes {

using galp_dataset_t = std::array<std::pair<std::string_view, std::string_view>, 6>;

class galp {
public:
	// Random two‑decimal GALP
	static constexpr std::string_view galp_one_vector {FLS_CMAKE_SOURCE_DIR "/data/generated/galp/galp/one_vector"};
	static constexpr std::string_view galp_single_column {FLS_CMAKE_SOURCE_DIR
	                                                      "/data/generated/galp/galp/single_column"};

	// Constant 1.00
	static constexpr std::string_view constant_one_vector {FLS_CMAKE_SOURCE_DIR
	                                                       "/data/generated/galp/constant/one_vector"};
	static constexpr std::string_view constant_single_column {FLS_CMAKE_SOURCE_DIR
	                                                          "/data/generated/galp/constant/single_column"};

	// Constant 1.00
	static constexpr std::string_view positive_inf_one_vector {FLS_CMAKE_SOURCE_DIR
	                                                           "/data/generated/galp/positive_inf/one_vector"};
	static constexpr std::string_view positive_inf_single_column {FLS_CMAKE_SOURCE_DIR
	                                                              "/data/generated/galp/positive_inf/single_column"};

	static constexpr galp_dataset_t dataset {
	    {
	        {"galp_one_vector", galp_one_vector},
	        {"galp_single_column", galp_single_column},
	        {"constant_one_vector", constant_one_vector},
	        {"constant_single_column", constant_single_column},
	        {"positive_inf_one_vector", positive_inf_one_vector},
	        {"positive_inf_single_column", positive_inf_single_column},
	        //
	    }
	    //
	};
};

} // namespace fastlanes

#endif // DATA_GALP_HPP
