#include "fls/primitive/patch/patch.hpp"
#include "fls/cuda/common.hpp"

namespace fastlanes {

template <typename PT>
void Patch<PT>::data_parallelize(const PT*       in_exc_arr,
                                 const uint16_t* in_pos_arr,
                                 n_t             n_exceptions,
                                 PT*             out_exc_arr,
                                 uint16_t*       out_offset,
                                 uint16_t*       out_pos_arr,
                                 uint8_t*        out_count) {
	constexpr auto N_LANES         = cuda::get_n_lanes<PT>();
	constexpr auto VALUES_PER_LANE = cuda::get_values_per_lane<PT>();

	// Intermediate arrays for reordering positions and exceptions
	PT       vec_exceptions[CFG::VEC_SZ];
	PT       vec_exceptions_positions[CFG::VEC_SZ];
	uint16_t n_local_exp[N_LANES];

	for (n_t lane = 0; lane < N_LANES; ++lane) {
		n_local_exp[lane] = 0;
	}

	// Split all exceptions into lanes
	for (n_t exc_idx {0}; exc_idx < n_exceptions; ++exc_idx) {
		PT       exception = in_exc_arr[exc_idx];
		uint16_t position  = in_pos_arr[exc_idx];

		n_t lane_idx             = position % N_LANES;
		n_t lane_exception_count = n_local_exp[lane_idx];
		++n_local_exp[lane_idx];
		vec_exceptions[lane_idx * VALUES_PER_LANE + lane_exception_count]           = exception;
		vec_exceptions_positions[lane_idx * VALUES_PER_LANE + lane_exception_count] = position;
	}

	n_t global_exc_idx = 0;
	for (n_t lane_idx {0}; lane_idx < N_LANES; ++lane_idx) {
		out_offset[lane_idx] = static_cast<uint16_t>(global_exc_idx);

		const n_t local_n_exp = n_local_exp[lane_idx];
		for (n_t exp_idx {0}; exp_idx < local_n_exp; ++exp_idx) {

			out_exc_arr[global_exc_idx] = vec_exceptions[lane_idx * VALUES_PER_LANE + exp_idx];
			out_pos_arr[global_exc_idx] =
			    static_cast<uint16_t>(vec_exceptions_positions[lane_idx * VALUES_PER_LANE + exp_idx]);
			++global_exc_idx;
		}

		out_count[lane_idx] = static_cast<uint8_t>(local_n_exp);
	}
}

template class Patch<flt_pt>;
template class Patch<dbl_pt>;

} // namespace fastlanes