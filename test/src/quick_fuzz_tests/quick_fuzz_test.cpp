#include "fls/connection.hpp"
#include "fls/csv/csv-parser/parser.hpp"
#include "fls/json/nlohmann/json.hpp"
#include "fls/std/filesystem.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fastlanes {

// ------------------------------------------------------------------
// Configuration: load fuzz parameters from JSON at
//   <repo>/test/src/quick_fuzz_tests/fuzz_config.json
// ------------------------------------------------------------------
namespace cfg {
struct Settings {
	int  num_cases = 10;
	int  base_seed = 42;
	char delim     = '|';
};

static Settings load() {
	Settings s;
	using fs::path;
	try {
		path config_path = path(FLS_CMAKE_SOURCE_DIR) / "test" / "src" / "quick_fuzz_tests" / "fuzz_config.json";
		std::ifstream in(config_path);
		if (in) {
			nlohmann::json j;
			in >> j;
			if (j.contains("num_cases")) {
				s.num_cases = j["num_cases"].get<int>();
			}
			if (j.contains("base_seed")) {
				s.base_seed = j["base_seed"].get<int>();
			}
			if (j.contains("delimiter")) {
				string d = j["delimiter"].get<string>();
				if (!d.empty()) {
					s.delim = d[0];
				}
			}
		} else {
			std::cerr << "[cfg] Warning: cannot open " << config_path << "; using defaults.\n";
		}
	} catch (const std::exception& ex) {
		std::cerr << "[cfg] Error reading fuzz_config.json: " << ex.what() << "; using defaults.\n";
	}
	return s;
}
} // namespace cfg

// ------------------------------------------------------------------
// Core routine: build CSV + schema, round-trip through FastLanes.
// Uses mixed types: integer or string, with occasional NULL.
// Each case in its own directory under tmp.
// ------------------------------------------------------------------
static bool run_roundtrip(int seed, int case_idx, char delim) {
	using fs::path;
	std::mt19937                       rng(seed);
	std::uniform_int_distribution<int> col_dist(1, 6);
	// Ensure at least 1 character so no empty string fields
	std::uniform_int_distribution<int> len_dist(1, 8);
	std::uniform_int_distribution<int> char_dist('a', 'z');
	std::uniform_int_distribution<int> int_dist(0, 1000);
	std::uniform_int_distribution<int> null_dist(0, 9); // 10% nulls
	const int                          cols = col_dist(rng);

	// Determine column types
	std::vector<bool>                  is_int(cols);
	std::uniform_int_distribution<int> type_dist(0, 1);
	for (int c = 0; c < cols; ++c) {
		if (type_dist(rng) == 1) {
			is_int[c] = true;
		} else {
			is_int[c] = false;
		}
	}

	// Build schema JSON
	nlohmann::json schema;
	schema["columns"] = nlohmann::json::array();
	for (int c = 0; c < cols; ++c) {
		nlohmann::json col;
		col["name"] = "col" + std::to_string(c);
		col["type"] = is_int[c] ? "integer" : "string";
		schema["columns"].push_back(col);
	}

	// Case-specific directory
	path repo_root = path(FLS_CMAKE_SOURCE_DIR);
	path case_dir  = repo_root / "tmp" / "fls_quick_fuzz" / ("case_" + std::to_string(case_idx));
	fs::remove_all(case_dir);
	fs::create_directories(case_dir);

	// File paths
	path csv_path    = case_dir / "generated.csv";
	path schema_path = case_dir / "schema.json";
	path fls_path    = case_dir / "data.fls";

	// Write schema
	{
		std::ofstream out(schema_path);
		out << schema.dump(2);
	}

	// Write CSV and capture first row for preview
	string first_row;
	{
		std::ofstream out(csv_path);
		for (int iter = 0; iter < 100; ++iter) {
			std::ostringstream row_ss;
			for (int c = 0; c < cols; ++c) {
				if (c > 0) {
					row_ss << delim;
				}
				if (is_int[c]) {
					if (null_dist(rng) == 0) {
						row_ss << "NULL";
					} else {
						row_ss << int_dist(rng);
					}
				} else {
					int len = len_dist(rng);
					for (int i = 0; i < len; ++i) {
						row_ss << static_cast<char>(char_dist(rng));
					}
				}
			}
			row_ss << '\n';
			string row = row_ss.str();
			if (iter == 0) {
				first_row = row;
			}
			out << row;
		}
	}

	// Optional preview of case 0
	if (case_idx == 0) {
		std::cout << "==== Case 0 Preview ====" << std::endl;
		std::cout << "Schema (cols=" << cols << "):\n" << schema.dump(2) << std::endl;
		std::cout << "First CSV line (delim '" << delim << "'):\n" << first_row << std::endl;
	}

	// Encode and decode
	Connection con1;
	con1.set_n_vectors_per_rowgroup(1).read_csv(case_dir).to_fls(fls_path);
	const auto& original_table = con1.get_table();

	Connection con2;
	auto       fls_reader    = con2.reset().read_fls(fls_path);
	auto       decoded_table = fls_reader->materialize();

	bool equal = (original_table == *decoded_table).is_equal;
	return equal;
}

// ------------------------------------------------------------------
// GTest: runs the configured number of deterministic fuzz cases.
// ------------------------------------------------------------------
TEST(QuickFuzz, RandomRoundTripsWithConfig) {
	const cfg::Settings s      = cfg::load();
	bool                all_ok = true;
	for (int i = 0; i < s.num_cases; ++i) {
		all_ok &= run_roundtrip(s.base_seed + i, i, s.delim);
	}
	EXPECT_TRUE(all_ok);

	// Cleanup entire fuzz tmp dir
	using fs::path;
	fs::remove_all(path(FLS_CMAKE_SOURCE_DIR) / "tmp" / "fls_quick_fuzz");
}

} // namespace fastlanes
