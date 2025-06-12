#include "fastlanes.hpp"          // provides FLS_CMAKE_SOURCE_DIR
#include "process.hpp"
#include "dct_extractor.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// Simple lowercase helper
static std::string to_lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
				   [](unsigned char c) { return std::tolower(c); });
	return s;
}

int main() {
	// Use FLS_CMAKE_SOURCE_DIR directly; no fastlanes::string namespace needed
	fs::path root   = fs::path(FLS_CMAKE_SOURCE_DIR);
	fs::path inDir  = root / "jpeg_playground" / "example" / "example_image";
	fs::path outDir = root / "jpeg_playground" / "tmp";
	fs::create_directories(outDir);

	int ok = 0, fail = 0;
	for (auto const& e : fs::directory_iterator(inDir)) {
		if (!e.is_regular_file()) continue;
		auto ext = to_lower(e.path().extension().string());
		if (ext != ".jpg" && ext != ".jpeg") continue;

		try {
			fs::path rec = jpeg_tools::process_jpeg(e.path(), outDir);

			// optional: DCT CSVs
			fs::path csv1 = outDir / (e.path().stem().string() + "_orig_dct.csv");
			fs::path csv2 = outDir / (e.path().stem().string() + "_rec_dct.csv");
			dct_extractor::extract_dct_to_csv(e.path(), csv1);
			dct_extractor::extract_dct_to_csv(rec, csv2);

			++ok;
		} catch (const std::exception& ex) {
			std::cerr << "Error processing " << e.path() << ": " << ex.what() << '\n';
			++fail;
		}
	}
	std::cout << "Done. Processed " << ok << ", Failed " << fail << ".\n";
	return 0;
}
