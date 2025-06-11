#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// suppress warnings in stb_image
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/fastlanes.hpp"
#include "stb_image.h"
#pragma clang diagnostic pop

namespace fs = std::filesystem;

// Helper function to convert string to lowercase
std::string to_lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

// Function to decode JPEG and append its raw pixel data to an existing CSV stream
bool append_decoded_jpeg_to_csv(const fs::path& jpeg_path, std::ofstream& csv_out_stream, bool write_header_info) {
	int            width, height, channels;
	unsigned char* img_data = stbi_load(jpeg_path.string().c_str(), &width, &height, &channels, 0);
	if (!img_data) {
		std::cerr << "Error: Could not decode JPEG file: " << jpeg_path << " - " << stbi_failure_reason() << std::endl;
		return false;
	}

	std::cout << "Processing " << jpeg_path.filename() << ": " << width << "x" << height << ", " << channels
	          << " channels." << std::endl;
	if (write_header_info) {
		csv_out_stream << "# BEGIN " << jpeg_path.filename().string() << "\n";
		csv_out_stream << "# Width: " << width << "\n";
		csv_out_stream << "# Height: " << height << "\n";
		csv_out_stream << "# Channels: " << channels << "\n";
	}

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			for (int c = 0; c < channels; ++c) {
				unsigned char value = img_data[(y * width + x) * channels + c];
				csv_out_stream << static_cast<int>(value);
				if (c < channels - 1)
					csv_out_stream << '|';
			}
			csv_out_stream << '\n';
		}
	}
	if (write_header_info)
		csv_out_stream << "# END " << jpeg_path.filename().string() << "\n";

	stbi_image_free(img_data);
	return true;
}

int main() {
	// Hardcoded directories
	fs::path    input_dir_path  = fastlanes::string {FLS_CMAKE_SOURCE_DIR} + "/jpeg_playground/example/example_image";
	fs::path    output_dir_path = fastlanes::string {FLS_CMAKE_SOURCE_DIR} + "/jpeg_playground/tmp";
	std::string output_csv_name = fastlanes::string {FLS_CMAKE_SOURCE_DIR} + "/jpeg_playground/decoded_pixel_data.csv";

	// Ensure input directory exists
	if (!fs::exists(input_dir_path) || !fs::is_directory(input_dir_path)) {
		std::cerr << "Error: Input directory does not exist or is not a directory: " << input_dir_path << std::endl;
		return 1;
	}

	// Create output directory if needed
	try {
		if (!fs::exists(output_dir_path)) {
			fs::create_directories(output_dir_path);
		}
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << std::endl;
		return 1;
	}

	fs::path      combined_csv_path = output_dir_path / output_csv_name;
	std::ofstream combined_csv_file(combined_csv_path, std::ios::out | std::ios::trunc);
	if (!combined_csv_file.is_open()) {
		std::cerr << "Error: Could not open combined CSV file: " << combined_csv_path << std::endl;
		return 1;
	}

	int files_processed = 0;
	int files_failed    = 0;
	for (const auto& entry : fs::directory_iterator(input_dir_path)) {
		if (entry.is_regular_file()) {
			auto ext = to_lower(entry.path().extension().string());
			if (ext == ".jpg" || ext == ".jpeg") {
				if (append_decoded_jpeg_to_csv(entry.path(), combined_csv_file, false))
					files_processed++;
				else
					files_failed++;
			}
		}
	}

	combined_csv_file.close();
	std::cout << "Processed: " << files_processed << ", Failed: " << files_failed << ".\n";
	return 0;
}
