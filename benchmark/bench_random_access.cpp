// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// benchmark/bench_random_access.cpp
// ────────────────────────────────────────────────────────
#include "benchmarker.hpp"
#include "benchmark_utils.hpp"

using namespace fastlanes; // NOLINT

class DecompressionTimeBenchmarker : public CompressionRatioBenchmarker {
public:
	explicit DecompressionTimeBenchmarker(const n_t n_repetitions)
	    : n_repetitions(n_repetitions) {
	}

	double bench_random_access(const path& dir_path) const {
		Connection conn;
		auto       fls_reader            = conn.reset().read_fls(dir_path / "data.fls");
		auto       first_rowgroup_reader = fls_reader->get_rowgroup_reader(0);

		auto start = std::chrono::high_resolution_clock::now();
		for (n_t repetition_idx {0}; repetition_idx < n_repetitions; repetition_idx++) {
			[[maybe_unused]] auto& expressions = first_rowgroup_reader->get_chunk(0);
		}
		const auto                                      end     = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double, std::milli> elapsed = end - start; // in milliseconds

		return elapsed.count();
	}

public:
	n_t n_repetitions {0};
};

void benchmark_random_access(dataset_view_t dataset_view) {
	const std::string result_file_path =
	    std::string(FLS_CMAKE_SOURCE_DIR) + "/benchmark/result/random_access/public_bi/fastlanes.csv";

        // Ensure the output directory exists
        create_directories(std::filesystem::path(result_file_path).parent_path());

        constexpr n_t n_repetition {1000};

        auto main_results = run_dataset(dataset_view, [&](const std::string_view file_path, const path& dir) {
                DecompressionTimeBenchmarker benchmarker {n_repetition};
                benchmarker.Write(file_path, dir);
                double random_access_ms = benchmarker.bench_random_access(dir);
                az_printer::green_cout << "-- Table " << file_path << " is benchmarked with time(ms): " << random_access_ms
                                       << std::endl;
                return random_access_ms;
        });

	// Write results to the CSV files
	std::ofstream csv_file(result_file_path);
	if (!csv_file.is_open()) {
		throw std::runtime_error("Failed to open the CSV file for writing.");
	}
	csv_file << "table_name,version,random_access_ms,n_repetition\n";
	for (const auto& [table_name, random_access_ms] : main_results) {
		csv_file << table_name << "," << Info::get_version() << "," << random_access_ms << "," << n_repetition << "\n";
	}
	csv_file.close();
	az_printer::green_cout << "-- Decompression times written to " << result_file_path << '\n';
}

int main() {
	benchmark_random_access(public_bi::dataset);
	return EXIT_SUCCESS;
}
