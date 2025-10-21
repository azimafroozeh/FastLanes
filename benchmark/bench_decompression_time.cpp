// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// benchmark/bench_decompression_time.cpp
// ────────────────────────────────────────────────────────
#include "benchmarker.hpp"
#include "benchmark_utils.hpp"

using namespace fastlanes; // NOLINT

class DecompressionTimeBenchmarker : public CompressionRatioBenchmarker {
public:
	explicit DecompressionTimeBenchmarker(const n_t n_repetitions)
	    : n_repetitions(n_repetitions) {
	}

	[[nodiscard]] double bench(const path& dir_path) const {
		Connection conn;

		auto fls_reader            = conn.reset().read_fls(dir_path / "data.fls");
		auto first_rowgroup_reader = fls_reader->get_rowgroup_reader(0);

		auto start = std::chrono::high_resolution_clock::now();
		for (n_t repetition_idx {0}; repetition_idx < n_repetitions; repetition_idx++) {
			for (n_t vec_idx {0}; vec_idx < first_rowgroup_reader->get_descriptor().m_n_vec(); vec_idx++) {
				first_rowgroup_reader->get_chunk(vec_idx);
			};
		}
		const auto                                      end     = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double, std::milli> elapsed = end - start; // in milliseconds

		return elapsed.count();
	}

public:
	n_t n_repetitions {0};
};

void bench_decompression(dataset_view_t dataset_view) {
	const std::string result_file_path =
	    std::string(FLS_CMAKE_SOURCE_DIR) + "/benchmark/result/decompression_time/public_bi/fastlanes.csv";

	// Ensure the output directory exists
	create_directories(std::filesystem::path(result_file_path).parent_path());

        const n_t n_repetition {1000};

        auto main_results = run_dataset(dataset_view, [&](const std::string_view file_path, const path& dir) {
                DecompressionTimeBenchmarker benchmarker {n_repetition};
                benchmarker.Write(file_path, dir);
                double ms = benchmarker.bench(dir);
                az_printer::green_cout << "-- Table " << file_path
                                       << " is benchmarked with time(ms): " << ms << std::endl;
                return ms;
        });

	// Compute the sum of decompression times
	double total_decompression_time_ms = 0;
	for (const auto& [_, decompression_time_ms] : main_results) {
		total_decompression_time_ms += decompression_time_ms;
	}

	// Write results to the CSV files
	std::ofstream csv_file(result_file_path);
	if (!csv_file.is_open()) {
		throw std::runtime_error("Failed to open the CSV file for writing.");
	}
	csv_file << "table_name,version,decompression_time_ms,n_repetition\n";
	for (const auto& [table_name, decompression_time_ms] : main_results) {
		csv_file << table_name << "," << Info::get_version() << "," << decompression_time_ms << "," << n_repetition
		         << "\n";
	}
	csv_file.close();
	az_printer::green_cout << "-- Decompression times written to " << result_file_path << '\n';

	// Print the total sum to the console
	az_printer::bold_magenta_cout << "-- Total decompression time (ms): " << total_decompression_time_ms << std::endl;
}

int main() {
	const auto start = std::chrono::high_resolution_clock::now();
	bench_decompression(public_bi::dataset);
	auto                                            end     = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double, std::milli> elapsed = end - start; // in milliseconds
	az_printer::bold_magenta_cout << "-- The whole benchmark time (ms): " << elapsed.count() << '\n';
	return EXIT_SUCCESS;
}
