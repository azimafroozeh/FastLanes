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
	// --- JPEG setup ---
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

	// --- CSV output ---
	std::ofstream out(csv);
	if (!out) {
		std::cerr << "Cannot open CSV " << csv << "\n";
		jpeg_finish_decompress(&dinfo);
		fclose(f);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	// // write header
	// out << "component,block_row,block_col";
	// for (int k = 0; k < 64; ++k) {
	// 	out << "|COL" << k;
	// }
	// out << '\n';

	// write data rows
	for (int c = 0; c < dinfo.num_components; ++c) {
		auto*    ci = &dinfo.comp_info[c];
		unsigned bw = ci->width_in_blocks, bh = ci->height_in_blocks;
		for (unsigned br = 0; br < bh; ++br) {
			JBLOCKARRAY row = dinfo.mem->access_virt_barray((j_common_ptr)&dinfo, coef[c], br, 1, FALSE);

			for (unsigned bc = 0; bc < bw; ++bc) {
				JCOEFPTR blk = row[0][bc];
				out << c << '|' << br << '|' << bc;
				for (int k = 0; k < 64; ++k) {
					out << '|' << blk[k];
				}
				out << '\n';
			}
		}
	}
	out.close();

	// --- JSON schema output ---
	fs::path      schema_path = csv.parent_path() / "schema.json";
	std::ofstream js(schema_path);
	if (!js) {
		std::cerr << "Cannot open JSON schema " << schema_path << "\n";
		// but we can still succeed at CSV extraction
	} else {
		js << "{\n"
		   << "  \"columns\": [\n";

		// first three columns
		js << "    { \"name\": \"component\", \"type\": \"smallint\" },\n"
		   << "    { \"name\": \"block_row\",  \"type\": \"smallint\" },\n"
		   << "    { \"name\": \"block_col\",  \"type\": \"smallint\" }";

		// COL0 .. COL63
		for (int k = 0; k < 64; ++k) {
			js << ",\n    { \"name\": \"COL" << k << "\", \"type\": \"smallint\" }";
		}
		js << "\n  ]\n"
		   << "}\n";
		js.close();
	}

	// clean up
	jpeg_finish_decompress(&dinfo);
	fclose(f);
	jpeg_destroy_decompress(&dinfo);
	return true;
}

} // namespace dct_extractor
#endif // DCT_EXTRACTOR_HPP
