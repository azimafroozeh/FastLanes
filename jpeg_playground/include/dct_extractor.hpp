#pragma once
#ifndef DCT_EXTRACTOR_HPP
#define DCT_EXTRACTOR_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <jpeglib.h>

namespace dct_extractor {
namespace fs = std::filesystem;

inline bool extract_dct_to_csv(const fs::path& jpg, const fs::path& csv) {
	jpeg_decompress_struct dinfo;
	jpeg_error_mgr         jerr;
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);

	FILE* f = std::fopen(jpg.string().c_str(), "rb");
	if (!f) {
		perror("fopen");
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jpeg_stdio_src(&dinfo, f);
	if (jpeg_read_header(&dinfo, TRUE) != JPEG_HEADER_OK) {
		std::cerr << "jpeg_read_header failed\n";
		fclose(f);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	jvirt_barray_ptr* coef = jpeg_read_coefficients(&dinfo);

	std::ofstream out(csv);
	if (!out) {
		std::cerr << "Cannot open CSV " << csv << "\n";
		jpeg_finish_decompress(&dinfo);
		fclose(f);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	out << "component,block_row,block_col";
	for (int k = 0; k < 64; ++k)
		out << ",c" << k;
	out << '\n';

	for (int c = 0; c < dinfo.num_components; ++c) {
		auto*    ci = &dinfo.comp_info[c];
		unsigned bw = ci->width_in_blocks, bh = ci->height_in_blocks;
		for (unsigned br = 0; br < bh; ++br) {
			JBLOCKARRAY row =
			    dinfo.mem->access_virt_barray((j_common_ptr)&dinfo, coef[c], static_cast<JDIMENSION>(br), 1, FALSE);

			for (unsigned bc = 0; bc < bw; ++bc) {
				JCOEFPTR blk = row[0][bc];
				out << c << '|' << br << '|' << bc;
				for (int k = 0; k < 64; ++k)
					out << '|' << blk[k];
				out << '\n';
			}
		}
	}

	jpeg_finish_decompress(&dinfo);
	fclose(f);
	jpeg_destroy_decompress(&dinfo);
	return true;
}
} // namespace dct_extractor
#endif
