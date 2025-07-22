#include "fls_tester.hpp"

namespace fastlanes {

TEST_F(FastLanesReaderTester, TEST_GALP_FLOAT_ONE_VECTOR) {
	TestCorrectness(galp::galp_one_vector, {OperatorToken::EXP_GALP_FLT});
}
TEST_F(FastLanesReaderTester, TEST_GALP_FLOAT_SINGLE_COLUMN) {
	TestCorrectness(galp::galp_single_column, {OperatorToken::EXP_GALP_FLT});
}
TEST_F(FastLanesReaderTester, TEST_GALP_FLOAT_CONSTANT_ONE_VECTOR) {
	TestCorrectness(galp::constant_one_vector, {OperatorToken::EXP_GALP_FLT});
}
TEST_F(FastLanesReaderTester, TEST_GALP_FLOAT_CONSTANT_SINGLE_COLUMN) {
	TestCorrectness(galp::constant_single_column, {OperatorToken::EXP_GALP_FLT});
}
TEST_F(FastLanesReaderTester, TEST_GALP_FLOAT_POSITIVE_INF_ONE_VECTOR) {
	TestCorrectness(galp::positive_inf_one_vector, {OperatorToken::EXP_GALP_FLT});
}
TEST_F(FastLanesReaderTester, TEST_GALP_FLOAT_POSITIVE_INF_SINGLE_COLUMN) {
	TestCorrectness(galp::positive_inf_single_column, {OperatorToken::EXP_GALP_FLT});
}
} // namespace fastlanes
