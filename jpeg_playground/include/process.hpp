#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "meta.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <jpeglib.h>
#include <limits>
#include <vector>

namespace jpeg_tools {
namespace fs = std::filesystem;

inline fs::path process_jpeg(const fs::path& src, const fs::path& outDir) {
	//----------------------------------------------------------------------
	// 1. Decode original --------------------------------------------------
	//----------------------------------------------------------------------
	jpeg_decompress_struct dinfo;
	jpeg_error_mgr         jerr;
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);

	for (int a = 0; a < 16; ++a)
		jpeg_save_markers(&dinfo, JPEG_APP0 + a, 0xFFFF);
	jpeg_save_markers(&dinfo, JPEG_COM, 0xFFFF);

	FILE* fp = std::fopen(src.string().c_str(), "rb");
	if (!fp)
		throw std::runtime_error("fopen");
	jpeg_stdio_src(&dinfo, fp);
	if (jpeg_read_header(&dinfo, TRUE) != JPEG_HEADER_OK)
		throw std::runtime_error("jpeg_read_header");

	dinfo.out_color_space = JCS_YCbCr;
	jpeg_start_decompress(&dinfo);

	JDIMENSION W = dinfo.output_width;
	JDIMENSION H = dinfo.output_height;
	int        C = dinfo.output_components;

	std::size_t raw_size = static_cast<std::size_t>(W) * static_cast<std::size_t>(H) * static_cast<std::size_t>(C);

	std::vector<unsigned char> raw(raw_size);
	std::vector<JSAMPROW>      rows(static_cast<std::size_t>(H));
	for (std::size_t y = 0; y < static_cast<std::size_t>(H); ++y)
		rows[y] = raw.data() + y * static_cast<std::size_t>(W) * static_cast<std::size_t>(C);

	while (dinfo.output_scanline < H) {
		JDIMENSION rem = H - dinfo.output_scanline;
		jpeg_read_scanlines(&dinfo, &rows[static_cast<std::size_t>(dinfo.output_scanline)], rem);
	}
	jpeg_finish_decompress(&dinfo);

	//----------------------------------------------------------------------
	// 2. Re-encode (copy tables + markers) --------------------------------
	//----------------------------------------------------------------------
	jpeg_compress_struct cinfo;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	unsigned char* outBuf = nullptr;
	unsigned long  outSz  = 0;
	jpeg_mem_dest(&cinfo, &outBuf, &outSz);

	jpeg_copy_critical_parameters(&dinfo, &cinfo);
	cinfo.optimize_coding  = TRUE;
	cinfo.in_color_space   = JCS_YCbCr;
	cinfo.input_components = C;

	for (jpeg_saved_marker_ptr mk = dinfo.marker_list; mk; mk = mk->next)
		jpeg_write_marker(&cinfo, mk->marker, mk->data, mk->data_length);

	jpeg_start_compress(&cinfo, TRUE);
	while (cinfo.next_scanline < cinfo.image_height) {
		JDIMENSION rownum = cinfo.next_scanline;
		jpeg_write_scanlines(&cinfo, &rows[static_cast<std::size_t>(rownum)], 1);
	}
	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);
	jpeg_destroy_decompress(&dinfo);
	std::fclose(fp);

	fs::path dst = outDir / (src.stem().string() + "_recompressed.jpg");
	{
		std::ofstream f(dst, std::ios::binary);
		f.write(reinterpret_cast<char*>(outBuf), static_cast<std::streamsize>(outSz));
	}
	std::free(outBuf);

	//----------------------------------------------------------------------
	// 3. Decode recompressed for PSNR -------------------------------------
	//----------------------------------------------------------------------
	jpeg_decompress_struct dinfo2;
	dinfo2.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo2);
	FILE* fp2 = std::fopen(dst.string().c_str(), "rb");
	jpeg_stdio_src(&dinfo2, fp2);
	jpeg_read_header(&dinfo2, TRUE);
	dinfo2.out_color_space = JCS_YCbCr;
	jpeg_start_decompress(&dinfo2);

	std::vector<unsigned char> raw2(raw_size);
	std::vector<JSAMPROW>      rows2(static_cast<std::size_t>(H));
	for (std::size_t y = 0; y < static_cast<std::size_t>(H); ++y)
		rows2[y] = raw2.data() + y * static_cast<std::size_t>(W) * static_cast<std::size_t>(C);

	while (dinfo2.output_scanline < H) {
		JDIMENSION rem = H - dinfo2.output_scanline;
		jpeg_read_scanlines(&dinfo2, &rows2[static_cast<std::size_t>(dinfo2.output_scanline)], rem);
	}
	jpeg_finish_decompress(&dinfo2);
	jpeg_destroy_decompress(&dinfo2);
	std::fclose(fp2);

	//----------------------------------------------------------------------
	// 4. PSNR + header dump ----------------------------------------------
	//----------------------------------------------------------------------
	double mse = 0.0;
	for (std::size_t i = 0; i < raw_size; ++i) {
		double d = double(raw[i]) - double(raw2[i]);
		mse += d * d;
	}
	// Fixed: cast raw_size to double
	mse /= static_cast<double>(raw_size);

	double psnr = (mse == 0.0) ? std::numeric_limits<double>::infinity() : 10.0 * std::log10((255.0 * 255.0) / mse);

	JpegMeta A, B;
	inspect_jpeg(src, A);
	inspect_jpeg(dst, B);
	print_meta_table(
	    src.filename().string(), A, detect_jpeg_process(src), dst.filename().string(), B, detect_jpeg_process(dst));
	std::cout << "PSNR: " << psnr << " dB\n\n";

	return dst;
}

} // namespace jpeg_tools

#endif // PROCESS_HPP
