#ifndef BENCHMARK_UTILS_HPP
#define BENCHMARK_UTILS_HPP

#include "benchmarker.hpp"
#include <algorithm>
#include <sstream>
#include <vector>

namespace fastlanes {

inline path make_thread_specific_dir() {
    std::ostringstream tid;
    tid << std::this_thread::get_id();
    return fastlanes_repo_data_path / "data" / "fls" / tid.str();
}

// Iterate over a dataset and run a benchmark function for each table.
// The benchmark function should accept (string_view file_path, const path& dir)
// and return a double representing the measured value.
// Results are returned sorted by table name.
template <typename BenchFunc>
std::vector<std::pair<std::string, double>>
run_dataset(dataset_view_t dataset_view, BenchFunc&& func) {
    std::vector<std::pair<std::string, double>> results;
    path dir = make_thread_specific_dir();
    for (const auto& [table_name, file_path] : dataset_view) {
        clear_directory(dir);
        double v = func(file_path, dir);
        results.emplace_back(std::string(table_name), v);
    }
    std::ranges::sort(results, [](const auto& a, const auto& b) { return a.first < b.first; });
    return results;
}

} // namespace fastlanes

#endif // BENCHMARK_UTILS_HPP
