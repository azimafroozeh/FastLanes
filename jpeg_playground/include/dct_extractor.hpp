#ifndef DCT_EXTRACTOR_HPP
#define DCT_EXTRACTOR_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <jpeglib.h>

namespace dct_extractor {
namespace fs = std::filesystem;

// JPEG standard zig-zag order: maps zig-zag index → natural block index
static constexpr int zigzag_map[64] = {0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42,
                                       3,  8,  12, 17, 25, 30, 41, 43, 9,  11, 18, 24, 31, 40, 44, 53,
                                       10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
                                       21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};

/**
 * extract_dct_to_csv_64cols
 * -------------------------
 * Reads a JPEG’s quantized 8×8 blocks and writes **one row per block**,
 * **64 comma-separated** coefficients in zig-zag order, no extra columns.
 */
inline bool extract_dct_to_csv_64cols(const fs::path& jpg, const fs::path& csv) {
	// 1) Set up libjpeg to read coefficients
	jpeg_decompress_struct dinfo;
	jpeg_error_mgr         jerr;
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);
	FILE* fp = std::fopen(jpg.string().c_str(), "rb");
	if (!fp) {
		perror("fopen");
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jpeg_stdio_src(&dinfo, fp);
	if (jpeg_read_header(&dinfo, TRUE) != JPEG_HEADER_OK) {
		std::cerr << "jpeg_read_header failed\n";
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jvirt_barray_ptr* coef_arrays = jpeg_read_coefficients(&dinfo);

	// 2) Open output CSV
	std::ofstream out(csv);
	if (!out) {
		std::cerr << "Cannot open CSV " << csv << "\n";
		jpeg_finish_decompress(&dinfo);
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	// 3) Iterate blocks: one row = 64 zig-zag coeffs
	for (int comp = 0; comp < dinfo.num_components; ++comp) {
		auto* ci = &dinfo.comp_info[comp];
		for (unsigned br = 0; br < ci->height_in_blocks; ++br) {
			JBLOCKARRAY row = dinfo.mem->access_virt_barray((j_common_ptr)&dinfo, coef_arrays[comp], br, 1, FALSE);
			for (unsigned bc = 0; bc < ci->width_in_blocks; ++bc) {
				JCOEFPTR blk = row[0][bc];
				for (int zz = 0; zz < 64; ++zz) {
					if (zz)
						out << ',';
					out << blk[zigzag_map[zz]];
				}
				out << '\n';
			}
		}
	}

	// 4) Cleanup
	out.close();
	jpeg_finish_decompress(&dinfo);
	fclose(fp);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

/**
 * extract_zigzag_blocks_csv
 * --------------------------
 * Reads a JPEG’s quantized 8×8 blocks and writes **one row per block**,
 * but each row is a **single quoted field** with 64 values separated by `|`.
 * This guarantees “one column per block” even in tools that split on commas.
 */
inline bool extract_zigzag_blocks_csv(const fs::path& jpg, const fs::path& csv) {
	// Same setup as above
	jpeg_decompress_struct dinfo;
	jpeg_error_mgr         jerr;
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);
	FILE* fp = std::fopen(jpg.string().c_str(), "rb");
	if (!fp) {
		perror("fopen");
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jpeg_stdio_src(&dinfo, fp);
	if (jpeg_read_header(&dinfo, TRUE) != JPEG_HEADER_OK) {
		std::cerr << "jpeg_read_header failed\n";
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jvirt_barray_ptr* coef_arrays = jpeg_read_coefficients(&dinfo);

	std::ofstream out(csv);
	if (!out) {
		std::cerr << "Cannot open CSV " << csv << "\n";
		jpeg_finish_decompress(&dinfo);
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	for (int comp = 0; comp < dinfo.num_components; ++comp) {
		auto* ci = &dinfo.comp_info[comp];
		for (unsigned br = 0; br < ci->height_in_blocks; ++br) {
			JBLOCKARRAY row = dinfo.mem->access_virt_barray((j_common_ptr)&dinfo, coef_arrays[comp], br, 1, FALSE);
			for (unsigned bc = 0; bc < ci->width_in_blocks; ++bc) {
				JCOEFPTR blk = row[0][bc];
				out << '"';
				for (int zz = 0; zz < 64; ++zz) {
					if (zz)
						out << '|';
					out << blk[zigzag_map[zz]];
				}
				out << '"' << '\n';
			}
		}
	}

	out.close();
	jpeg_finish_decompress(&dinfo);
	fclose(fp);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

// ----------  dct_extractor.hpp  (add after extract_dct_to_csv_64cols) --------
/**
 * extract_zigzag_stream_64rows
 * ----------------------------
 * Writes ONE column (no commas) where each 8×8 block is represented by
 * 64 successive lines (zig-zag order) followed by a blank separator line.
 * The file is opened in append mode so we never overwrite an existing CSV.
 */
inline bool extract_zigzag_stream_64rows(const fs::path& jpg, const fs::path& csv) {
	jpeg_decompress_struct dinfo;
	jpeg_error_mgr         jerr;
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);

	FILE* fp = std::fopen(jpg.string().c_str(), "rb");
	if (!fp) {
		perror("fopen");
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jpeg_stdio_src(&dinfo, fp);
	if (jpeg_read_header(&dinfo, TRUE) != JPEG_HEADER_OK) {
		std::cerr << "jpeg_read_header failed\n";
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jvirt_barray_ptr* coef = jpeg_read_coefficients(&dinfo);

	std::ofstream out(csv);
	if (!out) {
		std::cerr << "Cannot open CSV " << csv << '\n';
		jpeg_finish_decompress(&dinfo);
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	for (int c = 0; c < dinfo.num_components; ++c) {
		auto* ci = &dinfo.comp_info[c];
		for (unsigned br = 0; br < ci->height_in_blocks; ++br) {
			JBLOCKARRAY row = dinfo.mem->access_virt_barray((j_common_ptr)&dinfo, coef[c], br, 1, FALSE);
			for (unsigned bc = 0; bc < ci->width_in_blocks; ++bc) {
				JCOEFPTR blk = row[0][bc];
				for (int zz = 0; zz < 64; ++zz)
					out << blk[zigzag_map[zz]] << '\n';
				out << '\n'; // blank line = block separator
			}
		}
	}

	out.close();
	jpeg_finish_decompress(&dinfo);
	fclose(fp);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

} // namespace dct_extractor

#endif // DCT_EXTRACTOR_HPP
