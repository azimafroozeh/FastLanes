#include "dct_extractor.hpp" // two extractors
#include "fastlanes.hpp"     // FLS_CMAKE_SOURCE_DIR
#include "process.hpp"       // jpeg_tools::process_jpeg
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

/* helper: simple to-lower */
static std::string to_lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

/* one entry per output configuration */
struct Config {
	std::string dir;    // sub-directory name
	const char* schema; // JSON schema text
	bool (*extract)(const fs::path&, const fs::path&);
};

/* JSON schema strings */
static const char* schema64 =
    R"({
  "columns": [
)" /* + 64 entries added at runtime */;

static const char* schemaStream =
    R"({
  "columns": [
    { "name": "zigzag_coeffs", "type": "smallint" }
  ]
}
)";

int main() {
	fs::path root   = fs::path(FLS_CMAKE_SOURCE_DIR);
	fs::path inDir  = root / "jpeg_playground" / "example" / "example_image";
	fs::path outDir = root / "jpeg_playground" / "../data/image";

	/* configs we want */
	const Config cfgs[] = {{"zigzag64", schema64, dct_extractor::extract_dct_to_csv_64cols},
	                       {"zigzag_stream", schemaStream, dct_extractor::extract_zigzag_stream_64rows}};

	/* create config sub-dirs + schemas */
	for (auto const& cfg : cfgs) {
		fs::path d = outDir / cfg.dir;
		fs::create_directories(d);

		fs::path js = d / "schema.json";
		if (!fs::exists(js)) {
			std::ofstream s(js);
			if (cfg.dir == "zigzag64") { // build 64-col schema once
				s << "{\n  \"columns\": [\n";
				for (int k = 0; k < 64; ++k) {
					s << "    { \"name\": \"COL" << k << "\", \"type\": \"smallint\" }" << (k < 63 ? ",\n" : "\n");
				}
				s << "  ]\n}\n";
			} else {
				s << cfg.schema;
			}
		}
	}

	int ok = 0, fail = 0;

	/* main loop over JPEGs */
	for (auto const& entry : fs::directory_iterator(inDir)) {
		if (!entry.is_regular_file())
			continue;
		auto ext = to_lower(entry.path().extension().string());
		if (ext != ".jpg" && ext != ".jpeg")
			continue;

		try {
			/* recompress */
			fs::path    rec  = jpeg_tools::process_jpeg(entry.path(), outDir);
			std::string stem = entry.path().stem().string();

			/* run every extractor */
			for (auto const& cfg : cfgs) {
				fs::path outCsv = outDir / cfg.dir / (stem + ".csv");
				if (!cfg.extract(rec, outCsv))
					throw std::runtime_error("extract failed");
				std::cout << "Extracted " << outCsv << '\n';
			}
			++ok;
		} catch (const std::exception& ex) {
			std::cerr << "Error processing " << entry.path() << ": " << ex.what() << '\n';
			++fail;
		}
	}

	std::cout << "Done. Success: " << ok << ", Failed: " << fail << ".\n";
	return 0;
}
