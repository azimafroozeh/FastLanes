#include "fastlanes.hpp"
#include <algorithm>
#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <jpeglib.h> // libjpeg for encoding
#include <string>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma clang diagnostic pop

namespace fs = std::filesystem;

// Helper: lowercase a string
std::string to_lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

// Dump a byte as an 8-bit binary string
std::string byte_to_binary(unsigned char c) {
	return std::bitset<8>(c).to_string();
}

bool process_and_report(const fs::path& jpeg_path, const fs::path& output_dir, std::ofstream& report_stream) {
	// --- Load JPEG file raw data ---
	size_t         file_size = fs::file_size(jpeg_path);
	int            w, h, ch;
	unsigned char* img = stbi_load(jpeg_path.string().c_str(), &w, &h, &ch, 0);
	if (!img) {
		std::cerr << "Failed to decode " << jpeg_path << ": " << stbi_failure_reason() << "\n";
		return false;
	}

	size_t width    = static_cast<size_t>(w);
	size_t height   = static_cast<size_t>(h);
	size_t channels = static_cast<size_t>(ch);
	size_t raw_size = width * height * channels;

	// Print original JPEG info
	std::cout << "Original JPEG: " << jpeg_path.filename() << " | size=" << file_size << " B"
	          << " | dims=" << w << "x" << h << " | channels=" << ch << "\n";

	report_stream << "File: " << jpeg_path.filename() << "\n"
	              << " - Original JPEG size: " << file_size << " bytes\n"
	              << " - Dimensions: " << w << "x" << h << " (channels=" << ch << ")\n"
	              << " - Raw pixels size: " << raw_size << " bytes\n";

	// --- Re-encode with libjpeg into memory ---
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	unsigned char* outbuf  = nullptr;
	unsigned long  outsize = 0;
	jpeg_mem_dest(&cinfo, &outbuf, &outsize);

	cinfo.image_width      = static_cast<JDIMENSION>(width);
	cinfo.image_height     = static_cast<JDIMENSION>(height);
	cinfo.input_components = static_cast<int>(channels);
	cinfo.in_color_space   = (channels == 3 ? JCS_RGB : JCS_GRAYSCALE);

	// Use defaults + optimized Huffman
	jpeg_set_defaults(&cinfo);
	cinfo.optimize_coding = TRUE;
	jpeg_start_compress(&cinfo, TRUE);
	while (cinfo.next_scanline < cinfo.image_height) {
		JSAMPROW row_pointer = &img[cinfo.next_scanline * width * channels];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	// Write the recompressed JPEG to disk
	fs::path new_jpeg = output_dir / (jpeg_path.stem().string() + "_recompressed.jpg");
	{
		std::ofstream fout(new_jpeg, std::ios::binary | std::ios::out);
		fout.write(reinterpret_cast<char*>(outbuf), static_cast<std::streamsize>(outsize));
	}
	size_t new_file_size = fs::file_size(new_jpeg);

	// Compute compression ratio (raw/original)
	double ratio = double(raw_size) / double(file_size);

	// Report
	report_stream << " - Re-compressed JPEG: " << new_jpeg.filename() << " (" << new_file_size << " bytes)\n"
	              << " - Compression ratio: " << ratio << " (raw/original)\n"
	              << "First 10 pixel values in binary:\n";
	for (size_t i = 0; i < std::min<size_t>(10, raw_size); ++i) {
		report_stream << byte_to_binary(img[i]) << ((i + 1) % channels == 0 ? '\n' : ' ');
	}
	report_stream << "\n-------------------------------\n";

	// Echo recompressed info to console
	std::cout << "Recompressed JPEG: " << new_jpeg.filename() << " | size=" << new_file_size << " B\n"
	          << "Compression ratio (raw/original): " << ratio << "\n\n";

	stbi_image_free(img);
	free(outbuf);
	return true;
}

int main() {
	fs::path input_dir   = fastlanes::string {FLS_CMAKE_SOURCE_DIR} + "/jpeg_playground/example/example_image";
	fs::path output_dir  = fastlanes::string {FLS_CMAKE_SOURCE_DIR} + "/jpeg_playground/tmp";
	fs::path report_file = output_dir / "jpeg_compression_report.txt";

	if (!fs::exists(output_dir))
		fs::create_directories(output_dir);

	std::ofstream report(report_file, std::ios::out | std::ios::trunc);
	if (!report.is_open()) {
		std::cerr << "Cannot open report file: " << report_file << "\n";
		return 1;
	}

	int ok = 0, fail = 0;
	for (auto& e : fs::directory_iterator(input_dir)) {
		if (!e.is_regular_file())
			continue;
		auto ext = to_lower(e.path().extension().string());
		if (ext == ".jpg" || ext == ".jpeg") {
			if (process_and_report(e.path(), output_dir, report))
				ok++;
			else
				fail++;
		}
	}

	report << "Processed: " << ok << ", Failed: " << fail << "\n";
	report.close();

	std::cout << "Report written to " << report_file << "\n";
	return 0;
}
