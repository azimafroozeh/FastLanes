// test/src/primitive_tests/patch_test.cpp
// Google Test suite for fastlanes::Patch::data_parallelize
// Updated for API that returns **per‑lane counts (uint8_t) and offsets (uint16_t)**.
// Scenarios: zero exceptions, single exception, one‑per‑lane, 1024 sequential.

#include "fls/cuda/common.hpp"
#include "fls/primitive/patch/patch.hpp"
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

namespace fastlanes {

// -----------------------------------------------------------------------------
// Typed‑test fixture
// -----------------------------------------------------------------------------

template <typename PT>
class PatchDataParallelizeTest : public ::testing::Test {
protected:
	Patch<PT> patch;
};

using TestTypes = ::testing::Types<dbl_pt, flt_pt>; // test with double only (wide lane)
TYPED_TEST_SUITE(PatchDataParallelizeTest, TestTypes);

// -----------------------------------------------------------------------------
// 1. Zero exceptions
// -----------------------------------------------------------------------------

TYPED_TEST(PatchDataParallelizeTest, HandlesZeroExceptions) {
	using PT = TypeParam;

	constexpr n_t         n_exceptions = 0;
	std::vector<PT>       dummy_exc(1);
	std::vector<uint16_t> dummy_pos(1);

	std::vector<PT>       out_exc(1);
	std::vector<uint16_t> out_pos(1);
	const n_t             N_LANES = cuda::get_n_lanes<PT>();
	std::vector<uint16_t> lane_offsets(N_LANES, 0);
	std::vector<uint8_t>  local_n_exc(N_LANES, 0);

	Patch<PT>().data_parallelize(dummy_exc.data(),
	                             dummy_pos.data(),
	                             n_exceptions,
	                             out_exc.data(),
	                             lane_offsets.data(),
	                             out_pos.data(),
	                             local_n_exc.data());

	for (n_t lane = 0; lane < N_LANES; ++lane) {
		ASSERT_EQ(local_n_exc[lane], 0);
		ASSERT_EQ(lane_offsets[lane], static_cast<uint16_t>(0));
	}
}

// -----------------------------------------------------------------------------
// 2. Single exception routed correctly
// -----------------------------------------------------------------------------

TYPED_TEST(PatchDataParallelizeTest, SingleExceptionRoutedToCorrectLane) {
	using PT = TypeParam;

	const std::vector<PT>       in_exc       = {42};
	const std::vector<uint16_t> in_pos       = {3};
	constexpr n_t               n_exceptions = 1;

	std::vector<PT>       out_exc(n_exceptions);
	std::vector<uint16_t> out_pos(n_exceptions);
	const n_t             N_LANES = cuda::get_n_lanes<PT>();
	std::vector<uint16_t> lane_offsets(N_LANES, 0);
	std::vector<uint8_t>  local_n_exc(N_LANES, 0);

	Patch<PT>().data_parallelize(in_exc.data(),
	                             in_pos.data(),
	                             n_exceptions,
	                             out_exc.data(),
	                             lane_offsets.data(),
	                             out_pos.data(),
	                             local_n_exc.data());

	ASSERT_EQ(out_exc[0], in_exc[0]);

	for (n_t lane = 0; lane < N_LANES; ++lane) {
		if (lane == (in_pos[0] % N_LANES)) {
			ASSERT_EQ(local_n_exc[lane], 1);
		} else {
			ASSERT_EQ(local_n_exc[lane], 0);
		}
	}
}

// -----------------------------------------------------------------------------
// 3. One exception per lane
// -----------------------------------------------------------------------------

TYPED_TEST(PatchDataParallelizeTest, DistributesAcrossAllLanes) {
	using PT          = TypeParam;
	const n_t N_LANES = cuda::get_n_lanes<PT>();

	std::vector<PT>       in_exc;
	std::vector<uint16_t> in_pos;
	in_exc.reserve(N_LANES);
	in_pos.reserve(N_LANES);
	for (n_t lane = 0; lane < N_LANES; ++lane) {
		in_exc.push_back(static_cast<PT>(lane + 10));
		in_pos.push_back(static_cast<uint16_t>(lane));
	}

	const n_t             n_exceptions = N_LANES;
	std::vector<PT>       out_exc(n_exceptions);
	std::vector<uint16_t> out_pos(n_exceptions);
	std::vector<uint16_t> lane_offsets(N_LANES, 0);
	std::vector<uint8_t>  local_n_exc(N_LANES, 0);

	Patch<PT>().data_parallelize(in_exc.data(),
	                             in_pos.data(),
	                             n_exceptions,
	                             out_exc.data(),
	                             lane_offsets.data(),
	                             out_pos.data(),
	                             local_n_exc.data());

	for (n_t lane = 0; lane < N_LANES; ++lane) {
		ASSERT_EQ(local_n_exc[lane], 1);
		ASSERT_EQ(lane_offsets[lane], static_cast<uint16_t>(lane));
	}

	for (n_t i = 0; i < n_exceptions; ++i) {
		ASSERT_EQ(out_exc[i], in_exc[i]);
		ASSERT_EQ(out_pos[i], in_pos[i]);
	}
}

// -----------------------------------------------------------------------------
// 4. 1024 sequential exceptions (offsets wrap stress)
// -----------------------------------------------------------------------------

TYPED_TEST(PatchDataParallelizeTest, HandlesThousandTwentyFourSequential) {
	using PT                   = TypeParam;
	constexpr n_t n_exceptions = 1024;

	std::vector<PT>       in_exc(n_exceptions);
	std::vector<uint16_t> in_pos(n_exceptions);
	for (n_t i = 0; i < n_exceptions; ++i) {
		in_exc[i] = static_cast<PT>(i);
		in_pos[i] = static_cast<uint16_t>(i);
	}

	std::vector<PT>       out_exc(n_exceptions);
	std::vector<uint16_t> out_pos(n_exceptions);
	const n_t             N_LANES = cuda::get_n_lanes<PT>();
	std::vector<uint16_t> lane_offsets(N_LANES, 0);
	std::vector<uint8_t>  local_n_exp(N_LANES, 0);

	Patch<PT>().data_parallelize(in_exc.data(),
	                             in_pos.data(),
	                             n_exceptions,
	                             out_exc.data(),
	                             lane_offsets.data(),
	                             out_pos.data(),
	                             local_n_exp.data());

	n_t global_exc_idx = 0;
	for (n_t lane = 0; lane < N_LANES; ++lane) {
		ASSERT_EQ(lane_offsets[lane], static_cast<uint16_t>(global_exc_idx));
		global_exc_idx += local_n_exp[lane];
	}
}

} // namespace fastlanes