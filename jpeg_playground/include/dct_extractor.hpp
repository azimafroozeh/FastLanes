#ifndef DCT_EXTRACTOR_HPP
#define DCT_EXTRACTOR_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <jpeglib.h>

namespace dct_extractor {
namespace fs = std::filesystem;

/* ──────────────────────────────────────────────────────────────
 * JPEG zig-zag map: zig-zag index → natural block index
 * ──────────────────────────────────────────────────────────── */
static constexpr int zigzag_map[64] = {0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42,
                                       3,  8,  12, 17, 25, 30, 41, 43, 9,  11, 18, 24, 31, 40, 44, 53,
                                       10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
                                       21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};

/* =======================================================================
 * 1. extract_dct_to_csv_64cols
 *    Zig-zag order, 64 columns, pipe-delimited
 * ===================================================================== */
inline bool extract_dct_to_csv_64cols(const fs::path& jpg, const fs::path& csv) {
	// libjpeg setup
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

	// open CSV
	std::ofstream out(csv);
	if (!out) {
		std::cerr << "Cannot open " << csv << "\n";
		jpeg_finish_decompress(&dinfo);
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	// iterate components, blocks
	for (int c = 0; c < dinfo.num_components; ++c) {
		auto* ci = &dinfo.comp_info[c];
		for (unsigned br = 0; br < ci->height_in_blocks; ++br) {
			JBLOCKARRAY row = dinfo.mem->access_virt_barray((j_common_ptr)&dinfo, coef[c], br, 1, FALSE);
			for (unsigned bc = 0; bc < ci->width_in_blocks; ++bc) {
				JCOEFPTR blk = row[0][bc];
				for (int zz = 0; zz < 64; ++zz) {
					if (zz)
						out << '|'; // pipe delimiter
					out << blk[zigzag_map[zz]];
				}
				out << '\n';
			}
		}
	}

	// cleanup
	out.close();
	jpeg_finish_decompress(&dinfo);
	fclose(fp);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

/* =======================================================================
 * 2. extract_zigzag_blocks_csv
 *    Zig-zag order, single quoted field, internal pipes
 * ===================================================================== */
inline bool extract_zigzag_blocks_csv(const fs::path& jpg, const fs::path& csv) {
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
		std::cerr << "Cannot open " << csv << "\n";
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

/* =======================================================================
 * 3. extract_zigzag_stream_64rows
 *    Zig-zag order, 64 lines per block, no blank separator
 * ===================================================================== */
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
		std::cerr << "Cannot open " << csv << "\n";
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
				for (int zz = 0; zz < 64; ++zz) {
					out << blk[zigzag_map[zz]] << '\n';
				}
			}
		}
	}

	out.close();
	jpeg_finish_decompress(&dinfo);
	fclose(fp);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

/* =======================================================================
 * 4. extract_raw_dct_to_csv_64cols
 *    Natural order (0–63), 64 columns, pipe-delimited
 * ===================================================================== */
inline bool extract_raw_dct_to_csv_64cols(const fs::path& jpg, const fs::path& csv) {
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
		std::cerr << "Cannot open " << csv << "\n";
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
				for (int k = 0; k < 64; ++k) {
					if (k)
						out << '|';
					out << blk[k];
				}
				out << '\n';
			}
		}
	}

	out.close();
	jpeg_finish_decompress(&dinfo);
	fclose(fp);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

/* =======================================================================
 * 5. extract_raw_stream_64rows
 *    Natural order (0–63), 64 lines per block, no blank separator
 * ===================================================================== */
inline bool extract_raw_stream_64rows(const fs::path& jpg, const fs::path& csv) {
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
		std::cerr << "Cannot open " << csv << "\n";
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
				for (int k = 0; k < 64; ++k) {
					out << blk[k] << '\n';
				}
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
