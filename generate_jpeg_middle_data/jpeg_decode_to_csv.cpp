#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem> 
#include <cstdint>    
#include <algorithm>  


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

namespace fs = std::filesystem;

// Helper function to convert string to lowercase
std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// Function to decode JPEG and append its raw pixel data to an existing CSV stream
bool append_decoded_jpeg_to_csv(const fs::path& jpeg_path, std::ofstream& csv_out_stream, bool write_header_info) {
    int width, height, channels;
    // stbi_load returns unsigned char* pixel data.
    // The last argument (desired_channels):
    // 0 = use original channels
    // 1 = grayscale
    // 3 = RGB
    // 4 = RGBA
    // We use 0 to get the original format, then iterate through channels. 
    // jpeg is RGB in default
    unsigned char *img_data = stbi_load(jpeg_path.string().c_str(), &width, &height, &channels, 0);

    if (!img_data) {
        std::cerr << "Error: Could not decode JPEG file: " << jpeg_path << " - " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "Processing " << jpeg_path.filename() << ": " << width << "x" << height << ", " << channels << " channels." << std::endl;

    if (write_header_info) {
        csv_out_stream << "# BEGIN " << jpeg_path.filename().string() << "\n";
        csv_out_stream << "# Width: " << width << "\n";
        csv_out_stream << "# Height: " << height << "\n";
        csv_out_stream << "# Channels: " << channels << "\n";
    }
    
    // Write pixel data to CSV
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                // Calculate the index in the 1D array
                unsigned char pixel_component_value = img_data[(y * width + x) * channels + c];
                /*
                   The data is stored row by row, channel by channel 
                    (e.g., R,
                           G,
                           B, 
                           R, ...)
                */ 
                // csv_out_stream << static_cast<int>(pixel_component_value) << "\n";
                
                /*
                    each row three data 
                    (e.g., R,G,B, 
                           R,G,B, ...)
                */ 
                csv_out_stream << static_cast<int>(pixel_component_value);
                if (c < channels - 1) {
                    csv_out_stream << "|"; 
                }
                
            }
            csv_out_stream << "\n"; 
        }
    }


    
    if (write_header_info) {
         csv_out_stream << "# END " << jpeg_path.filename().string() << "\n";
    }

    stbi_image_free(img_data); // IMPORTANT: Free the image data allocated by stbi_load

    std::cout << "Successfully appended decoded pixel data from " << jpeg_path.filename() << " to combined CSV." << std::endl;
    return true;
}


int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_directory> <output_directory> [output_csv_filename.csv]" << std::endl;
        return 1;
    }

    fs::path input_dir_path(argv[1]);
    fs::path output_dir_path(argv[2]);
    
    std::string output_csv_name = "decoded_pixel_data.csv"; // Default name
    if (argc == 4) {
        output_csv_name = argv[3];
        if (fs::path(output_csv_name).extension() != ".csv") {
            output_csv_name += ".csv"; // Ensure .csv extension
        }
    }

    if (!fs::exists(input_dir_path)) {
        std::cerr << "Error: Input directory does not exist: " << input_dir_path << std::endl;
        return 1;
    }
    if (!fs::is_directory(input_dir_path)) {
        std::cerr << "Error: Input path is not a directory: " << input_dir_path << std::endl;
        return 1;
    }

    try {
        if (!fs::exists(output_dir_path)) {
            if (fs::create_directories(output_dir_path)) {
                std::cout << "Created output directory: " << output_dir_path << std::endl;
            } else {
                std::cerr << "Error: Could not create output directory: " << output_dir_path << std::endl;
                return 1;
            }
        } else if (!fs::is_directory(output_dir_path)) {
             std::cerr << "Error: Output path exists but is not a directory: " << output_dir_path << std::endl;
            return 1;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    }

    fs::path combined_csv_path = output_dir_path / output_csv_name;

    std::ofstream combined_csv_file(combined_csv_path, std::ios::out | std::ios::trunc);
    if (!combined_csv_file.is_open()) {
        std::cerr << "Error: Could not create or open combined CSV file: " << combined_csv_path << std::endl;
        return 1;
    }
    std::cout << "Opened combined CSV file for writing: " << combined_csv_path << std::endl;

    int files_processed = 0;
    int files_failed = 0;

    for (const auto& entry : fs::directory_iterator(input_dir_path)) {
        if (entry.is_regular_file()) {
            fs::path current_file_path = entry.path();
            std::string extension = to_lower(current_file_path.extension().string());

            if (extension == ".jpg" || extension == ".jpeg") {
                if (append_decoded_jpeg_to_csv(current_file_path, combined_csv_file, false)) {
                    files_processed++;
                } else {
                    files_failed++;
                }
            }
        }
    }

    combined_csv_file.close();
    std::cout << "Closed combined CSV file: " << combined_csv_path << std::endl;

    std::cout << "\nProcessing complete." << std::endl;
    std::cout << "Total JPEG files processed and appended: " << files_processed << std::endl;
    std::cout << "Files failed to process: " << files_failed << std::endl;
    if (files_processed > 0) {
        std::cout << "All decoded pixel data written to: " << combined_csv_path << std::endl;
    }

    return 0;
}