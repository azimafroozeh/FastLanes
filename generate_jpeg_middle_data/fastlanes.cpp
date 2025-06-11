#include "data/fastlanes_data.hpp"
#include "fastlanes.hpp"
#include "fls/connection.hpp"
#include "fls/printer/az_printer.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace fastlanes; // NOLINT

int main(int argc, char* argv[]) {

	try {
		auto       con1             = connect();

		if (argc != 4) {
			std::cerr << "Usage: " << argv[0] << " <input_directory> <output_fls_filename.fls> <output_csv_filename.csv>" << std::endl;
			return 1;
		}

		fs::path example_dir_path(argv[1]);
		fs::path fls_file_path(argv[2]);
		fs::path csv_file_path(argv[3]);
		

		if (!fs::exists(example_dir_path)) {
			std::cerr << "Error: Input directory does not exist: " << example_dir_path << std::endl;
			return 1;
		}
		if (!fs::is_directory(example_dir_path)) {
			std::cerr << "Error: Input path is not a directory: " << example_dir_path << std::endl;
			return 1;
		}


		// Remove only data.fls file
		if (exists(fls_file_path)) {
			std::error_code ec;
			fs::remove(fls_file_path, ec);
			if (ec) {
				std::cerr << "Failed to remove data.fls: " << ec.message() << std::endl;
			}
		}

		// Step 1: Read the CSV file from the specified directory path
		con1->set_n_vectors_per_rowgroup(64).read_csv(example_dir_path);

		// Step 2: Write the data to the FastLanes file format in the specified directory
		con1->to_fls(fls_file_path);

		// Step 3: Get a FastLanes reader for the previously stored data
		Connection con2;
		const auto fls_reader = con2.reset().read_fls(fls_file_path);

		// Step 4: Write data to CSV
		if (exists(csv_file_path)) {
			std::filesystem::remove(csv_file_path);
		}
		fls_reader->to_csv(csv_file_path);

		exit(EXIT_SUCCESS);
	} catch (std::exception& ex) {
		az_printer::bold_red_cout << "-- Error: " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
