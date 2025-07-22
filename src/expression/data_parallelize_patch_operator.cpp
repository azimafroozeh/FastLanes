#include "fls/expression/data_parallelize_patch_operator.hpp"
#include "fls/common/string.hpp"
#include "fls/cor/lyt/buf.hpp"
#include "fls/expression/alp_expression.hpp"
#include "fls/expression/decoding_operator.hpp"
#include "fls/expression/interpreter.hpp"
#include "fls/expression/physical_expression.hpp"
#include "fls/primitive/patch/patch.hpp"
#include "fls/reader/column_view.hpp"
#include "fls/reader/segment.hpp"
#include "fls/unffor.hpp"
#include <cstring>

namespace fastlanes {
/*--------------------------------------------------------------------------------------------------------------------*\
 * enc_data_parallel_patch_opr
\*--------------------------------------------------------------------------------------------------------------------*/
template <typename PT>
enc_data_parallel_patch_opr<PT>::enc_data_parallel_patch_opr(const PhysicalExpr& expr,
                                                             const col_pt&       column,
                                                             ColumnDescriptorT&  column_descriptor,
                                                             InterpreterState&   state) {
	visit(overloaded {
	          [&](const sp<enc_alp_opr<PT>>& opr) {
		          in_exc_arr    = opr->exc_arr;
		          in_pos_arr    = opr->pos_arr;
		          n_exception_p = &opr->alp_state.n_exceptions;
	          },
	          [&](std::monostate&) { FLS_UNREACHABLE(); },
	          [&](auto& arg) { FLS_UNREACHABLE_WITH_TYPE(arg); },
	      },
	      expr.operators[state.cur_operator++]);

	out_count_segment  = make_unique<Segment>();
	out_offset_segment = make_unique<Segment>();
}

template <typename PT>
void enc_data_parallel_patch_opr<PT>::DataParallelize() {
	Patch<PT>::data_parallelize(
	    in_exc_arr, in_pos_arr, *n_exception_p, out_exc_arr, out_offset, out_pos_arr, out_count);
	for (n_t exc_idx {0}; exc_idx < *n_exception_p; ++exc_idx) {
		in_exc_arr[exc_idx] = out_exc_arr[exc_idx];
		in_pos_arr[exc_idx] = out_pos_arr[exc_idx];
	}
	out_count_segment->Flush(out_count, 32);
	out_offset_segment->Flush(out_offset, 64);
}

template <typename PT>
void enc_data_parallel_patch_opr<PT>::MoveSegments(vector<up<Segment>>& segments) {
	segments.push_back(std::move(out_count_segment));
	segments.push_back(std::move(out_offset_segment));
}

template struct enc_data_parallel_patch_opr<dbl_pt>;
template struct enc_data_parallel_patch_opr<flt_pt>;

} // namespace fastlanes
