// quick_fuzz_test.cpp
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

// ------------------------------------------------------------------
// Helper: minimal Base64 encoder for embedding JPEG bytes in CSV
// ------------------------------------------------------------------
static const std::string b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

static std::string base64_encode(const std::vector<uint8_t>& buf) {
	std::string out;
	int         val = 0, valb = -6;
	for (uint8_t c : buf) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(b64_chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6)
		out.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4)
		out.push_back('=');
	return out;
}

namespace fastlanes {
namespace cfg {

// ------------------------------------------------------------------
// Fuzz settings read from fuzz_config.json (optional).
// ------------------------------------------------------------------
struct Settings {
	int  num_cases = 10;
	int  base_seed = 42;
	char delim     = '|';
};

inline Settings load() {
	Settings s;
	using fs::path;
	try {
		path          cfg_path = path(FLS_CMAKE_SOURCE_DIR) / "test" / "src" / "quick_fuzz_tests" / "fuzz_config.json";
		std::ifstream in(cfg_path);
		if (in) {
			nlohmann::json j;
			in >> j;
			if (j.contains("num_cases"))
				s.num_cases = j["num_cases"];
			if (j.contains("base_seed"))
				s.base_seed = j["base_seed"];
			if (j.contains("delimiter")) {
				auto d = j["delimiter"].get<std::string>();
				if (!d.empty())
					s.delim = d[0];
			}
		} else {
			std::cerr << "[cfg] Warning: cannot open " << cfg_path << "; using defaults.\n";
		}
	} catch (const std::exception& ex) {
		std::cerr << "[cfg] Error parsing fuzz_config.json: " << ex.what() << "; using defaults.\n";
	}
	return s;
}

} // namespace cfg

// ------------------------------------------------------------------
// One round-trip: generate CSV + schema → FastLanes → back.
// Column types chosen with rng()%3 (portable across libstdc++/libc++).
// No guarantee that a jpeg column will appear.
// ------------------------------------------------------------------
static bool run_roundtrip(int seed, int case_idx, char delim) {
	using fs::path;
	std::mt19937 rng(seed);

	std::uniform_int_distribution<int> col_dist(1, 6);
	std::uniform_int_distribution<int> len_dist(1, 8);
	std::uniform_int_distribution<int> char_dist('a', 'z');
	std::uniform_int_distribution<int> int_dist(0, 1000);
	std::uniform_int_distribution<int> null_dist(0, 9); // 10 % NULLs
	std::uniform_int_distribution<int> byte_dist(0, 255);

	const int cols = col_dist(rng);

	// 0 = integer, 1 = string, 2 = jpeg (binary)
	std::vector<int> col_type(cols);
	for (int c = 0; c < cols; ++c) {
		col_type[c] = static_cast<int>(rng() % 3); // portable seq.
	}

	// Build JSON schema
	nlohmann::json schema;
	schema["columns"] = nlohmann::json::array();
	for (int c = 0; c < cols; ++c) {
		nlohmann::json col;
		col["name"] = "col" + std::to_string(c);
		col["type"] = (col_type[c] == 0) ? "integer" : (col_type[c] == 1) ? "string" : "jpeg";
		schema["columns"].push_back(col);
	}

	// Paths
	path root   = path(FLS_CMAKE_SOURCE_DIR);
	path case_d = root / "tmp" / "fls_quick_fuzz" / ("case_" + std::to_string(case_idx));
	fs::remove_all(case_d);
	fs::create_directories(case_d);
	path img_d = case_d / "images";
	fs::create_directories(img_d);

	path csv_p    = case_d / "generated.csv";
	path schema_p = case_d / "schema.json";
	path fls_p    = case_d / "data.fls";

	// Write schema
	{ std::ofstream(schema_p) << schema.dump(2); }

	// Write CSV rows
	std::ofstream csv(csv_p);
	for (int row = 0; row < 100; ++row) {
		std::ostringstream row_ss;
		for (int c = 0; c < cols; ++c) {
			if (c)
				row_ss << delim;

			switch (col_type[c]) {
			case 0: { // integer
				if (null_dist(rng) == 0)
					row_ss << "NULL";
				else
					row_ss << int_dist(rng);
			} break;

			case 1: { // string
				int len = len_dist(rng);
				for (int i = 0; i < len; ++i)
					row_ss << static_cast<char>(char_dist(rng));
			} break;

			case 2: { // jpeg (binary)
				std::vector<uint8_t> img;
				img.push_back(0xFF);
				img.push_back(0xD8); // SOI
				const uint8_t jfif[] = {0xFF,
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
				img.insert(img.end(), std::begin(jfif), std::end(jfif));
				int pay_len = len_dist(rng) * 4;
				for (int i = 0; i < pay_len; ++i)
					img.push_back(static_cast<uint8_t>(byte_dist(rng)));
				img.push_back(0xFF);
				img.push_back(0xD9); // EOI

				std::string fname = "img_r" + std::to_string(row) + "_c" + std::to_string(c) + ".jpg";
				std::ofstream(img_d / fname, std::ios::binary).write(reinterpret_cast<char*>(img.data()), img.size());

				row_ss << base64_encode(img);
			} break;
			}
		}
		csv << row_ss.str() << '\n';
	}

	// Case-0 preview
	if (case_idx == 0) {
		std::cout << "==== Case 0 Preview ====\n"
		          << "Schema:\n"
		          << schema.dump(2) << "\n---\n"
		          << "First CSV line:\n";
		std::ifstream in(csv_p);
		std::string   line;
		std::getline(in, line);
		std::cout << line << '\n';
	}

	// FastLanes round-trip
	Connection con1;
	con1.set_n_vectors_per_rowgroup(1).read_csv(case_d).to_fls(fls_p);
	const auto& original = con1.get_table();

	Connection con2;
	auto       decoded = con2.reset().read_fls(fls_p)->materialize();

	return (original == *decoded).is_equal;
}

// ------------------------------------------------------------------
// GTest driver
// ------------------------------------------------------------------
TEST(QuickFuzz, RandomRoundTripsWithConfig) {
	const cfg::Settings s  = cfg::load();
	bool                ok = true;
	for (int i = 0; i < s.num_cases; ++i)
		ok &= run_roundtrip(s.base_seed + i, i, s.delim);
	EXPECT_TRUE(ok);

	using fs::path;
	fs::remove_all(path(FLS_CMAKE_SOURCE_DIR) / "tmp" / "fls_quick_fuzz");
}

} // namespace fastlanes
