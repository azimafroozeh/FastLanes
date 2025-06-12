#pragma once
#ifndef META_HPP
#define META_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <jpeglib.h>
#include <sstream> // <-- REQUIRED for std::ostringstream

namespace jpeg_tools {
namespace fs = std::filesystem;

//==============================================================================
// JPEG metadata + process detection
//==============================================================================

enum class JpegProcess {
	Baseline_Huffman,
	Extended_Huffman,
	Progressive_Huffman,
	Lossless_Huffman,
	Baseline_Arithmetic,
	Extended_Arithmetic,
	Progressive_Arithmetic,
	Lossless_Arithmetic,
	Unknown
};

struct JpegMeta {
	std::uintmax_t     size = 0;
	int                w = 0, h = 0, comps = 0;
	J_COLOR_SPACE      cs   = JCS_UNKNOWN;
	bool               prog = false;
	std::array<int, 4> h_samp {}, v_samp {};
	int                qt = 0, hDC = 0, hAC = 0;
	bool               jfif = false, exif = false;
};

//------------------------------------------------------------------------------
// Inspect a JPEG header, fill JpegMeta
//------------------------------------------------------------------------------
inline bool inspect_jpeg(const fs::path& p, JpegMeta& m) {
	if (!fs::is_regular_file(p))
		return false;
	m.size = fs::file_size(p);

	jpeg_decompress_struct dinfo;
	jpeg_error_mgr         jerr;
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);

	for (int a = 0; a < 16; ++a)
		jpeg_save_markers(&dinfo, JPEG_APP0 + a, 0xFFFF);
	jpeg_save_markers(&dinfo, JPEG_COM, 0xFFFF);

	FILE* fp = std::fopen(p.string().c_str(), "rb");
	if (!fp) {
		perror("fopen");
		jpeg_destroy_decompress(&dinfo);
		return false;
	}
	jpeg_stdio_src(&dinfo, fp);
	if (jpeg_read_header(&dinfo, TRUE) != JPEG_HEADER_OK) {
		std::cerr << "jpeg_read_header failed: " << p << '\n';
		fclose(fp);
		jpeg_destroy_decompress(&dinfo);
		return false;
	}

	m.w     = static_cast<int>(dinfo.image_width);
	m.h     = static_cast<int>(dinfo.image_height);
	m.comps = dinfo.num_components;
	m.cs    = dinfo.jpeg_color_space;
	m.prog  = dinfo.progressive_mode;
	for (std::size_t i = 0; i < static_cast<std::size_t>(dinfo.num_components); ++i) {
		m.h_samp[i] = dinfo.comp_info[i].h_samp_factor;
		m.v_samp[i] = dinfo.comp_info[i].v_samp_factor;
	}
	for (int i = 0; i < NUM_QUANT_TBLS; ++i)
		if (dinfo.quant_tbl_ptrs[i])
			++m.qt;
	for (int i = 0; i < NUM_HUFF_TBLS; ++i) {
		if (dinfo.dc_huff_tbl_ptrs[i])
			++m.hDC;
		if (dinfo.ac_huff_tbl_ptrs[i])
			++m.hAC;
	}

	const int APP1 = JPEG_APP0 + 1;
	for (jpeg_saved_marker_ptr mk = dinfo.marker_list; mk; mk = mk->next) {
		if (mk->marker == JPEG_APP0 && mk->data_length >= 5 && !std::memcmp(mk->data, "JFIF", 5))
			m.jfif = true;
		if (mk->marker == APP1 && mk->data_length >= 6 && !std::memcmp(mk->data, "Exif\0\0", 6))
			m.exif = true;
	}

	jpeg_destroy_decompress(&dinfo);
	fclose(fp);
	return true;
}

//------------------------------------------------------------------------------
// Map enum → human-readable string
//------------------------------------------------------------------------------
inline const char* to_string(JpegProcess p) {
	switch (p) {
	case JpegProcess::Baseline_Huffman:
		return "Baseline Huffman";
	case JpegProcess::Extended_Huffman:
		return "Extended Huffman";
	case JpegProcess::Progressive_Huffman:
		return "Progressive Huffman";
	case JpegProcess::Lossless_Huffman:
		return "Lossless Huffman";
	case JpegProcess::Baseline_Arithmetic:
		return "Baseline Arithmetic";
	case JpegProcess::Extended_Arithmetic:
		return "Extended Arithmetic";
	case JpegProcess::Progressive_Arithmetic:
		return "Progressive Arithmetic";
	case JpegProcess::Lossless_Arithmetic:
		return "Lossless Arithmetic";
	default:
		return "Unknown";
	}
}

//------------------------------------------------------------------------------
// Detect JPEG “process” (baseline/prog/lossless + coder type)
//------------------------------------------------------------------------------
inline JpegProcess detect_jpeg_process(const fs::path& p) {
	std::ifstream in(p, std::ios::binary);
	if (!in)
		return JpegProcess::Unknown;

	bool          saw_dht = false, saw_dac = false;
	unsigned char byte, marker;

	if (!in.read((char*)&byte, 1) || !in.read((char*)&byte, 1) || byte != 0xD8)
		return JpegProcess::Unknown;

	while (true) {
		do {
			if (!in.read((char*)&byte, 1))
				return JpegProcess::Unknown;
		} while (byte != 0xFF);
		do {
			if (!in.read((char*)&marker, 1))
				return JpegProcess::Unknown;
		} while (marker == 0xFF);

		if (marker == 0xDA || marker == 0xD9)
			break; // SOS / EOI
		if (marker == 0xC4)
			saw_dht = true; // DHT
		if (marker == 0xCC)
			saw_dac = true; // DAC

		unsigned char hi, lo;
		in.read((char*)&hi, 1);
		in.read((char*)&lo, 1);
		unsigned len = (unsigned)hi << 8 | lo;

		switch (marker) {
		case 0xC0:
			return JpegProcess::Baseline_Huffman;
		case 0xC1:
			return JpegProcess::Extended_Huffman;
		case 0xC2:
			return JpegProcess::Progressive_Huffman;
		case 0xC3:
			return JpegProcess::Lossless_Huffman;
		case 0xC8:
			return JpegProcess::Baseline_Arithmetic;
		case 0xC9:
			return JpegProcess::Extended_Arithmetic;
		case 0xCA:
			return JpegProcess::Progressive_Arithmetic;
		case 0xCB:
			return JpegProcess::Lossless_Arithmetic;
		default:
			in.seekg(static_cast<std::streamoff>(len - 2), std::ios::cur);
		}
	}
	if (saw_dht)
		return JpegProcess::Baseline_Huffman;
	if (saw_dac)
		return JpegProcess::Baseline_Arithmetic;
	return JpegProcess::Unknown;
}

//------------------------------------------------------------------------------
// Pretty-print side-by-side metadata table
//------------------------------------------------------------------------------
inline void print_meta_table(const std::string& aName,
                             const JpegMeta&    A,
                             JpegProcess        pa,
                             const std::string& bName,
                             const JpegMeta&    B,
                             JpegProcess        pb) {
	auto pad = [](int w, const std::string& s) {
		std::ostringstream oss;
		oss << std::left << std::setw(w) << s;
		return oss.str();
	};

	auto row = [&](const char* lbl, const std::string& a, const std::string& b) {
		std::cout << pad(22, lbl) << " | " << pad(14, a) << " | " << pad(14, b) << '\n';
	};

	std::cout << '\n'
	          << pad(22, "") << " | " << pad(14, aName) << " | " << pad(14, bName) << '\n'
	          << std::string(22 + 2 * 14 + 5, '-') << '\n';

	row("File size (B)", std::to_string(A.size), std::to_string(B.size));
	row("Dimensions", std::to_string(A.w) + 'x' + std::to_string(A.h), std::to_string(B.w) + 'x' + std::to_string(B.h));
	row("# components", std::to_string(A.comps), std::to_string(B.comps));

	auto csLabel = [](J_COLOR_SPACE cs) {
		switch (cs) {
		case JCS_YCbCr:
			return "YCbCr";
		case JCS_RGB:
			return "RGB";
		case JCS_GRAYSCALE:
			return "Gray";
		default:
			return "Other";
		}
	};
	row("Colour space", csLabel(A.cs), csLabel(B.cs));
	row("Progressive", A.prog ? "Yes" : "No", B.prog ? "Yes" : "No");

	auto samp = [](const JpegMeta& m) {
		return std::to_string(m.h_samp[0]) + 'x' + std::to_string(m.v_samp[0]);
	};
	row("Sampling", samp(A), samp(B));
	row("Quant tables", std::to_string(A.qt), std::to_string(B.qt));
	row("Huff DC tbl", std::to_string(A.hDC), std::to_string(B.hDC));
	row("Huff AC tbl", std::to_string(A.hAC), std::to_string(B.hAC));
	row("JFIF", A.jfif ? "Yes" : "No", B.jfif ? "Yes" : "No");
	row("EXIF", A.exif ? "Yes" : "No", B.exif ? "Yes" : "No");
	row("Encoding", to_string(pa), to_string(pb));

	std::cout << std::string(22 + 2 * 14 + 5, '-') << "\n\n";
}

} // namespace jpeg_tools
#endif // META_HPP
