#include "dct_extractor.hpp" // all five extractors
#include "fastlanes.hpp"     // FLS_CMAKE_SOURCE_DIR
#include "process.hpp"       // jpeg_tools::process_jpeg
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// simple lowercase helper
static std::string to_lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

// one entry per output configuration
struct Config {
	std::string dir;    // sub-directory name
	const char* schema; // JSON schema text
	bool (*extract)(const fs::path&, const fs::path&);
};

// JSON schema for any 64-column CSV (never actually written, used only as a marker)
static const char* schema64 =
    R"({
  "columns": [
})"; // properly closed raw-string + semicolon

// JSON schema for any single-column CSV
static const char* schemaStream =
    R"({
  "columns": [
    { "name": "coeff", "type": "smallint" }
  ]
}
)";

int main() {
	fs::path root   = fs::path(FLS_CMAKE_SOURCE_DIR);
	fs::path inDir  = root / "jpeg_playground" / "example" / "example_image";
	fs::path outDir = root / "jpeg_playground" / "../data/image";

	// all five extractor variants
	const Config cfgs[] = {
	    // zig-zag, 64-column
	    {"zigzag64", schema64, dct_extractor::extract_dct_to_csv_64cols},
	    // zig-zag, quoted single-field
	    {"zigzag_blocks", schemaStream, dct_extractor::extract_zigzag_blocks_csv},
	    // zig-zag, stream 64 rows
	    {"zigzag_stream", schemaStream, dct_extractor::extract_zigzag_stream_64rows},
	    // raw, natural-order 64-column
	    {"raw64", schema64, dct_extractor::extract_raw_dct_to_csv_64cols},
	    // raw, stream 64 rows
	    {"raw_stream", schemaStream, dct_extractor::extract_raw_stream_64rows},
	    {"zigzag_index_stream", schemaStream, dct_extractor::extract_zigzag_cols_stream},
	    {"raw_index", schemaStream, dct_extractor::extract_raw_index_stream},
	};

	// create each config sub-dir + write its schema.json once
	for (auto const& cfg : cfgs) {
		fs::path d = outDir / cfg.dir;
		fs::create_directories(d);

		fs::path js = d / "schema.json";
		if (!fs::exists(js)) {
			std::ofstream s(js);
			if (cfg.schema == schema64) {
				// build 64-column schema
				s << "{\n  \"columns\": [\n";
				for (int k = 0; k < 64; ++k) {
					s << "    { \"name\": \"COL" << k << "\", \"type\": \"smallint\" }" << (k < 63 ? ",\n" : "\n");
				}
				s << "  ]\n}\n";
			} else {
				// single-column schema
				s << cfg.schema;
			}
		}
	}

	int ok = 0, fail = 0;

	// process each JPEG
	for (auto const& entry : fs::directory_iterator(inDir)) {
		if (!entry.is_regular_file())
			continue;
		auto ext = to_lower(entry.path().extension().string());
		if (ext != ".jpg" && ext != ".jpeg")
			continue;

		try {
			// recompress to a new JPEG
			fs::path    rec  = jpeg_tools::process_jpeg(entry.path(), outDir);
			std::string stem = entry.path().stem().string();

			// run every extractor variant
			for (auto const& cfg : cfgs) {
				fs::path outCsv = outDir / cfg.dir / (stem + ".csv");
				if (!cfg.extract(rec, outCsv)) {
					throw std::runtime_error("extract failed: " + cfg.dir);
				}
				std::cout << "Extracted [" << cfg.dir << "] " << outCsv << "\n";
			}
			++ok;
		} catch (const std::exception& ex) {
			std::cerr << "Error processing " << entry.path() << ": " << ex.what() << "\n";
			++fail;
		}
	}

	std::cout << "Done. Success: " << ok << ", Failed: " << fail << ".\n";
	return 0;
}
