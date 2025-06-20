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
	if (valb > -6) {
		out.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
	}
	while (out.size() % 4) {
		out.push_back('=');
	}
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
// One round-trip: generate CSV+schema, round-trip through FastLanes.
// Column types chosen with rng()%3 (portable), then if no JPEG at all,
// we force one—so every run triggers the missing-type failure.
// ------------------------------------------------------------------
static bool run_roundtrip(int seed, int case_idx, char delim) {
	using fs::path;
	std::mt19937 rng(seed);

	// Use rng()%N everywhere for portability
	auto uniform = [&](int a, int b) {
		return a + static_cast<int>(rng() % (b - a + 1));
	};

	const int cols = uniform(1, 6);

	// 0 = integer, 1 = string, 2 = jpeg
	std::vector<int> col_type(cols);
	bool             saw_jpeg = false;
	for (int c = 0; c < cols; ++c) {
		col_type[c] = static_cast<int>(rng() % 3);
		if (col_type[c] == 2)
			saw_jpeg = true;
	}
	// Guarantee at least one jpeg column
	if (!saw_jpeg) {
		col_type[static_cast<int>(rng() % cols)] = 2;
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

	// Prepare directories
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
		std::ostringstream ss;
		for (int c = 0; c < cols; ++c) {
			if (c)
				ss << delim;
			switch (col_type[c]) {
			case 0: // integer
				if ((rng() % 10) == 0)
					ss << "NULL";
				else
					ss << uniform(0, 1000);
				break;
			case 1: { // string
				int L = uniform(1, 8);
				for (int i = 0; i < L; ++i)
					ss << static_cast<char>(uniform('a', 'z'));
			} break;
			case 2: { // jpeg
				std::vector<uint8_t> img;
				img.push_back(0xFF);
				img.push_back(0xD8); // SOI
				const uint8_t hdr[] = {0xFF,
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
				img.insert(img.end(), std::begin(hdr), std::end(hdr));
				int PL = uniform(1, 8) * 4;
				for (int i = 0; i < PL; ++i)
					img.push_back(static_cast<uint8_t>(rng() % 256));
				img.push_back(0xFF);
				img.push_back(0xD9); // EOI

				// Dump to file for inspection
				std::string fn = "img_r" + std::to_string(row) + "_c" + std::to_string(c) + ".jpg";
				std::ofstream(img_d / fn, std::ios::binary).write(reinterpret_cast<char*>(img.data()), img.size());

				ss << base64_encode(img);
			} break;
			}
		}
		csv << ss.str() << "\n";
	}

	// Preview every case to stderr
	{
		std::ifstream in(csv_p);
		std::string   first;
		std::getline(in, first);
		std::cerr << "==== Case " << case_idx << " Preview ====\n"
		          << "Schema:\n"
		          << schema.dump(2) << "\n"
		          << "First CSV line:\n"
		          << first << "\n\n";
	}

	// Round-trip through FastLanes
	Connection con1;
	con1.set_n_vectors_per_rowgroup(1).read_csv(case_d).to_fls(fls_p);
	const auto& tbl1 = con1.get_table();

	Connection con2;
	auto       tbl2 = con2.reset().read_fls(fls_p)->materialize();

	return (tbl1 == *tbl2).is_equal;
}

// ------------------------------------------------------------------
// GTest driver
// ------------------------------------------------------------------
TEST(QuickFuzz, RandomRoundTripsWithConfig) {
	const cfg::Settings S      = cfg::load();
	bool                all_ok = true;
	for (int i = 0; i < S.num_cases; ++i) {
		all_ok &= run_roundtrip(S.base_seed + i, i, S.delim);
	}
	EXPECT_TRUE(all_ok);

	// Cleanup
	using fs::path;
	fs::remove_all(path(FLS_CMAKE_SOURCE_DIR) / "tmp" / "fls_quick_fuzz");
}

} // namespace fastlanes
