#ifndef FLS_EXPRESSION_DATA_PARALLEL_PATCH_OPERATOR_HPP
#define FLS_EXPRESSION_DATA_PARALLEL_PATCH_OPERATOR_HPP

#include "fls/cuda/common.hpp"
#include "fls/reader/segment.hpp"
#include "fls/table/chunk.hpp"
#include "fls/table/rowgroup.hpp"

namespace fastlanes {
/*--------------------------------------------------------------------------------------------------------------------*/
class Segment;
struct ColumnDescriptorT;
class PhysicalExpr;
struct InterpreterState;
class ColumnView;
struct InterpreterState;
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------*\
 * enc_alp_opr
\*--------------------------------------------------------------------------------------------------------------------*/
template <typename PT>
struct enc_data_parallel_patch_opr {
	explicit enc_data_parallel_patch_opr(const PhysicalExpr& expr,
	                                     const col_pt&       column,
	                                     ColumnDescriptorT&  column_descriptor,
	                                     InterpreterState&   state);

	void DataParallelize();
	void MoveSegments(vector<up<Segment>>& segments);

public:
	// arrays
	alignas(64) uint8_t out_count[cuda::get_n_lanes<PT>()];
	alignas(64) uint16_t out_offset[cuda::get_n_lanes<PT>()];
	alignas(64) uint16_t out_pos_arr[CFG::VEC_SZ];
	alignas(64) PT out_exc_arr[CFG::VEC_SZ];

	PT*         in_exc_arr;
	uint16_t*   in_pos_arr;
	uint16_t*   n_exception_p;
	up<Segment> out_count_segment;
	up<Segment> out_offset_segment;
};

} // namespace fastlanes

#endif // FLS_EXPRESSION_DATA_PARALLEL_PATCH_OPERATOR_HPP
