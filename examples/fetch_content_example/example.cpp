#include "fastlanes.hpp"

using namespace fastlanes; // NOLINT

int main() {

	try {
		auto       con1             = connect();
		const path example_dir_path = string("data");
		const path fls_file_path    = path {"."} / "data.fls";
		const path csv_file_path    = path{"."} / "decoded_by_fastlanes.csv";

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
		return EXIT_FAILURE;
	}
}
