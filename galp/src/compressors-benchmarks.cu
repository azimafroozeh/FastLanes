// benchmark_runner.cpp – self‑contained driver that replicates the shell script logic
// Compile with C++17 or newer, e.g.:
//   g++ -std=c++17 benchmark_runner.cpp -o compressors-benchmarks $(CUDA_LIBS) ...
// -----------------------------------------------------------------------------
#include "engine/data.cuh"
#include "engine/enums.cuh"
#include "flsgpu/consts.cuh"
#include "nvcomp/benchmark-compressors.cuh"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Utility helpers (unchanged from the original source)
// -----------------------------------------------------------------------------
std::string extract_filename(const std::string& path) {
	const std::size_t pos      = path.find_last_of("/");
	std::string       filename = (pos == std::string::npos) ? path : path.substr(pos + 1);
	const std::size_t extPos   = filename.find_last_of('.');
	return (extPos == std::string::npos) ? filename : filename.substr(0, extPos);
}

struct CLIArgs {
	enums::DataType               data_type;
	enums_nvcomp::ComparisonType  comparison_type;
	enums_nvcomp::CompressionType decompressor_enum;
	std::string                   file_path;
	int                           n_values;
};

CLIArgs parse_cli_args(int argc, char* argv[]) {
	if (argc != 6) {
		std::cerr << "Usage: " << argv[0]
		          << " <data_type> <comparison_enum:string> <decompressor_enum:string> <data_path> <vector_count>\n";
		throw std::invalid_argument("Incorrect CLI argument count");
	}

	int32_t idx = 0;
	CLIArgs args;
	args.data_type         = enums::string_to_data_type(argv[++idx]);
	args.comparison_type   = enums_nvcomp::string_to_comparison_type(argv[++idx]);
	args.decompressor_enum = enums_nvcomp::string_to_compression_type(argv[++idx]);
	args.file_path         = argv[++idx];
	args.n_values          = std::atoi(argv[++idx]) * consts::VALUES_PER_VECTOR;
	return args;
}

// Template benchmark launcher (unchanged)
template <typename T>
void execute_benchmark(const CLIArgs& args) {
	auto [data, count] = data::arrays::read_file_as<T>(args.file_path, args.n_values);

	// Deterministic but arbitrary search value (matches shell script)
	uint32_t u                   = 0x4100f9db;
	const T  value_to_search_for = *reinterpret_cast<T*>(&u);

	BenchmarkResult result;
	for (int i = 0; i < 2; ++i) {
		if (args.comparison_type == enums_nvcomp::ComparisonType::DECOMPRESSION_QUERY &&
		    args.decompressor_enum == enums_nvcomp::THRUST) {
			result = benchmark_thrust<T>(data, count, value_to_search_for);
		} else if (args.decompressor_enum == enums_nvcomp::ALP || args.decompressor_enum == enums_nvcomp::GALP) {
			result = benchmark_alp<T>(args.comparison_type, args.decompressor_enum, data, count, value_to_search_for);
		} else {
			result = benchmark_hwc<T>(args.comparison_type, args.decompressor_enum, data, count, value_to_search_for);
		}
	}

	result.log_result(
	    args.comparison_type, args.decompressor_enum, count * sizeof(T), extract_filename(args.file_path));

	delete data;
}

// -----------------------------------------------------------------------------
// Hard‑coded benchmark matrix (replicates test‑scripts/run.sh logic)
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
	constexpr int VECTOR_COUNT = 25'600; // matches the shell script constant

	// ---------------------------------------------------------------------------
	// Fallback: keep original single‑case CLI if exactly 5 user arguments present
	// ---------------------------------------------------------------------------
	if (argc == 6) {
		CLIArgs args = parse_cli_args(argc, argv);
		switch (args.data_type) {
		case enums::DataType::F32:
			execute_benchmark<float>(args);
			break;
		case enums::DataType::F64:
			execute_benchmark<double>(args);
			break;
		default:
			throw std::invalid_argument("Unsupported data type");
		}
		return 0;
	}

	// ---------------------------------------------------------------------------
	// Automatic discovery of .bin files inside data/float and data/double
	// ---------------------------------------------------------------------------
	std::vector<std::string> float_files, double_files;
	std::filesystem::path    float_path = FLS_GALP_SOURCE_DIR "/data/floats";
	for (const auto& entry : std::filesystem::directory_iterator(float_path)) {
		if (entry.is_regular_file() && entry.path().extension() == ".bin")
			float_files.push_back(entry.path().string());
	}
	std::filesystem::path double_path = FLS_GALP_SOURCE_DIR "/data/doubles";
	for (const auto& entry : std::filesystem::directory_iterator(double_path)) {
		if (entry.is_regular_file() && entry.path().extension() == ".bin")
			double_files.push_back(entry.path().string());
	}

	if (float_files.empty() && double_files.empty()) {
		std::cerr << "No .bin test files found. Please populate data/float and data/double." << std::endl;
		return 1;
	}

	// ---------------------------------------------------------------------------
	// Enumerations matching the shell‑script variables
	// ---------------------------------------------------------------------------
	const std::vector<enums_nvcomp::ComparisonType> COMPARISONS = {enums_nvcomp::ComparisonType::DECOMPRESSION,
	                                                               enums_nvcomp::ComparisonType::DECOMPRESSION_QUERY};

	const std::vector<enums_nvcomp::CompressionType> COMPRESSORS = {enums_nvcomp::ALP,
	                                                                enums_nvcomp::GALP,
	                                                                enums_nvcomp::BITCOMP,
	                                                                enums_nvcomp::BITCOMP_SPARSE,
	                                                                enums_nvcomp::LZ4,
	                                                                enums_nvcomp::ZSTD,
	                                                                enums_nvcomp::DEFLATE,
	                                                                enums_nvcomp::GDEFLATE,
	                                                                enums_nvcomp::SNAPPY};

	// ---------------------------------------------------------------------------
	// (1) Thrust path – only DECOMPRESSION_QUERY variant, both F32 and F64
	// ---------------------------------------------------------------------------
	auto run_thrust = [&](enums::DataType dtype, const std::vector<std::string>& files) {
		for (const auto& path : files) {
			CLIArgs args;
			args.data_type         = dtype;
			args.comparison_type   = enums_nvcomp::ComparisonType::DECOMPRESSION_QUERY;
			args.decompressor_enum = enums_nvcomp::THRUST;
			args.file_path         = path;
			args.n_values          = VECTOR_COUNT * consts::VALUES_PER_VECTOR;

			if (dtype == enums::DataType::F32)
				execute_benchmark<float>(args);
			else
				execute_benchmark<double>(args);
		}
	};

	run_thrust(enums::DataType::F32, float_files);
	run_thrust(enums::DataType::F64, double_files);

	// ---------------------------------------------------------------------------
	// (2) Full matrix for Floats (F32)
	// ---------------------------------------------------------------------------
	for (auto cmp : COMPARISONS) {
		for (auto decomp : COMPRESSORS) {
			for (const auto& path : float_files) {
				CLIArgs args {enums::DataType::F32, cmp, decomp, path, VECTOR_COUNT * consts::VALUES_PER_VECTOR};
				execute_benchmark<float>(args);
			}
		}
	}

	// ---------------------------------------------------------------------------
	// (3) Full matrix for Doubles (F64)
	// ---------------------------------------------------------------------------
	for (auto cmp : COMPARISONS) {
		for (auto decomp : COMPRESSORS) {
			for (const auto& path : double_files) {
				CLIArgs args {enums::DataType::F64, cmp, decomp, path, VECTOR_COUNT * consts::VALUES_PER_VECTOR};
				execute_benchmark<double>(args);
			}
		}
	}

	return 0;
}
