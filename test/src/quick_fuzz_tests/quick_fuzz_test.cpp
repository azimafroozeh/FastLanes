#include "fls/connection.hpp"
#include "fls/csv/csv-parser/parser.hpp"
#include "fls/json/nlohmann/json.hpp"
#include "fls/std/filesystem.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ------------------------------------------------------------------
// Helper: minimal Base64 encoder for embedding JPEG bytes in CSV
// ------------------------------------------------------------------
static const std::string b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

static std::string base64_encode(const std::vector<uint8_t>& data) {
	std::string ret;
	int         val = 0, valb = -6;
	for (uint8_t c : data) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			ret.push_back(b64_chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) {
		ret.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
	}
	while (ret.size() % 4) {
		ret.push_back('=');
	}
	return ret;
}

namespace fastlanes {
namespace cfg {

// ------------------------------------------------------------------
// Configuration: load fuzz parameters from JSON at
//   <repo>/test/src/quick_fuzz_tests/fuzz_config.json
// ------------------------------------------------------------------
struct Settings {
	int  num_cases = 10;
	int  base_seed = 42;
	char delim     = '|';
};

// Inline load() definition so there's no missing-symbol at link time
inline Settings load() {
	Settings s;
	using fs::path;
	try {
		path config_path = path(FLS_CMAKE_SOURCE_DIR) / "test" / "src" / "quick_fuzz_tests" / "fuzz_config.json";
		std::ifstream in(config_path);
		if (in) {
			nlohmann::json j;
			in >> j;
			if (j.contains("num_cases"))
				s.num_cases = j["num_cases"].get<int>();
			if (j.contains("base_seed"))
				s.base_seed = j["base_seed"].get<int>();
			if (j.contains("delimiter")) {
				auto d = j["delimiter"].get<std::string>();
				if (!d.empty())
					s.delim = d[0];
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
// Core routine: build CSV + schema, round-trip through FastLanes,
// now with integer, string, or jpeg columns.
// ------------------------------------------------------------------
static bool run_roundtrip(int seed, int case_idx, char delim) {
	using fs::path;

	std::mt19937                       rng(seed);
	std::uniform_int_distribution<int> col_dist(1, 6);
	std::uniform_int_distribution<int> len_dist(1, 8);
	std::uniform_int_distribution<int> char_dist('a', 'z');
	std::uniform_int_distribution<int> int_dist(0, 1000);
	std::uniform_int_distribution<int> null_dist(0, 9); // 10% nulls

	const int cols = col_dist(rng);

	// 0 = integer, 1 = string, 2 = jpeg
	std::vector<int>                   col_type(cols);
	std::uniform_int_distribution<int> type_dist(0, 2);
	for (int c = 0; c < cols; ++c) {
		col_type[c] = type_dist(rng);
	}

	// Build schema JSON
	nlohmann::json schema;
	schema["columns"] = nlohmann::json::array();
	for (int c = 0; c < cols; ++c) {
		nlohmann::json col;
		col["name"] = "col" + std::to_string(c);
		switch (col_type[c]) {
		case 0:
			col["type"] = "integer";
			break;
		case 1:
			col["type"] = "string";
			break;
		case 2:
			col["type"] = "jpeg";
			break;
		}
		schema["columns"].push_back(col);
	}

	// Prepare case directory
	path repo_root = path(FLS_CMAKE_SOURCE_DIR);
	path case_dir  = repo_root / "tmp" / "fls_quick_fuzz" / ("case_" + std::to_string(case_idx));
	fs::remove_all(case_dir);
	fs::create_directories(case_dir);
	path img_dir = case_dir / "images";
	fs::create_directories(img_dir);

	path csv_path    = case_dir / "generated.csv";
	path schema_path = case_dir / "schema.json";
	path fls_path    = case_dir / "data.fls";

	// Write schema
	{
		std::ofstream out(schema_path);
		out << schema.dump(2);
	}

	// Write CSV
	{
		std::ofstream out(csv_path);
		for (int iter = 0; iter < 100; ++iter) {
			std::ostringstream row_ss;
			for (int c = 0; c < cols; ++c) {
				if (c > 0)
					row_ss << delim;

				if (col_type[c] == 0) {
					// Integer or NULL
					if (null_dist(rng) == 0) {
						row_ss << "NULL";
					} else {
						row_ss << int_dist(rng);
					}

				} else if (col_type[c] == 1) {
					// Random string
					int len = len_dist(rng);
					for (int i = 0; i < len; ++i) {
						row_ss << static_cast<char>(char_dist(rng));
					}

				} else {
					// JPEG: build minimal JFIF + random payload + EOI
					std::vector<uint8_t> img_data;
					img_data.push_back(0xFF);
					img_data.push_back(0xD8);
					// Minimal JFIF header
					const uint8_t header[] = {0xFF,
					                          0xE0,
					                          0x00,
					                          0x10,
					                          'J',
					                          'F',
					                          'I',
					                          'F',
					                          0x00,
					                          0x01,
					                          0x02,
					                          0x00,
					                          0x00,
					                          0x01,
					                          0x00,
					                          0x01,
					                          0x00,
					                          0x00};
					img_data.insert(img_data.end(), std::begin(header), std::end(header));
					// Random payload
					int                                payload_len = len_dist(rng) * 4;
					std::uniform_int_distribution<int> byte_dist(0, 255);
					for (int i = 0; i < payload_len; ++i) {
						img_data.push_back(static_cast<uint8_t>(byte_dist(rng)));
					}
					// EOI
					img_data.push_back(0xFF);
					img_data.push_back(0xD9);

					// Write .jpg file (optional, for inspection)
					std::string fname    = "img_r" + std::to_string(iter) + "_c" + std::to_string(c) + ".jpg";
					path        img_path = img_dir / fname;
					{
						std::ofstream img_out(img_path, std::ios::binary);
						img_out.write(reinterpret_cast<char*>(img_data.data()), img_data.size());
					}

					// Embed base64 in CSV
					row_ss << base64_encode(img_data);
				}
			}
			row_ss << '\n';
			out << row_ss.str();
		}
	}

	// Preview case 0 if desired
	if (case_idx == 0) {
		std::cout << "==== Case 0 Preview ====\n"
		          << "Schema (cols=" << cols << "):\n"
		          << schema.dump(2) << "\n---\n"
		          << "First CSV line (delim '" << delim << "'):\n";
		std::ifstream in(csv_path);
		std::string   line;
		std::getline(in, line);
		std::cout << line << "\n";
	}

	// Round-trip through FastLanes
	Connection con1;
	con1.set_n_vectors_per_rowgroup(1).read_csv(case_dir).to_fls(fls_path);
	const auto& original_table = con1.get_table();

	Connection con2;
	auto       fls_reader    = con2.reset().read_fls(fls_path);
	auto       decoded_table = fls_reader->materialize();

	return (original_table == *decoded_table).is_equal;
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

	// Cleanup
	using fs::path;
	fs::remove_all(path(FLS_CMAKE_SOURCE_DIR) / "tmp" / "fls_quick_fuzz");
}

} // namespace fastlanes
