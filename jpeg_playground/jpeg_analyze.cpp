/*****************************************************************************
 * jpeg_analyze.cpp
 *   • Decodes every *.jpg in example_image/
 *   • Re-encodes with libjpeg (optimised Huffman)
 *   • Computes PSNR
 *   • Compares ORIGINAL vs. RECOMPRESSED header details side-by-side
 *   • Shows which JPEG “process” (baseline/progressive/lossless + coder)
 *   • Writes human-readable report + console output
 *****************************************************************************/

#include "fastlanes.hpp"
#include <array>
#include <bitset>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <jpeglib.h>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

/* ─────────────────── stb_image with warnings muted ─────────────────────── */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#define STBI_NO_16BIT
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
/* ────────────────────────────────────────────────────────────────────────── */

/*--------------------------- helper: lowercase ----------------------------*/
static std::string to_lower(std::string s) {
	for (char& c : s)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

/*--------------------------- helper: byte→binary --------------------------*/
static std::string byte_bits(unsigned char b) {
	return std::bitset<8>(b).to_string();
}

/*==========================================================================
 * 1. JPEG-HEADER INTROSPECTION  (re-usable for both images)
 *==========================================================================*/
struct JpegMeta {
	std::uintmax_t     size  = 0; // bytes on disk
	int                w     = 0;
	int                h     = 0;
	int                comps = 0;
	J_COLOR_SPACE      cs    = JCS_UNKNOWN;
	bool               prog  = false;
	std::array<int, 4> h_samp {};
	std::array<int, 4> v_samp {};
	int                qt   = 0;
	int                hDC  = 0;
	int                hAC  = 0;
	bool               jfif = false;
	bool               exif = false;
};

static bool inspect_jpeg(const fs::path& p, JpegMeta& m) {
	if (!fs::is_regular_file(p))
		return false;
	m.size = fs::file_size(p);

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr         jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	/* save APPn markers for JFIF/EXIF detection */
	for (int app = 0; app < 16; ++app)
		jpeg_save_markers(&cinfo, JPEG_APP0 + app, 0xFFFF);

	FILE* fp = std::fopen(p.string().c_str(), "rb");
	if (!fp) {
		perror("fopen");
		jpeg_destroy_decompress(&cinfo);
		return false;
	}
	jpeg_stdio_src(&cinfo, fp);
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		std::cerr << "jpeg_read_header failed: " << p << "\n";
		fclose(fp);
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	m.w     = static_cast<int>(cinfo.image_width);
	m.h     = static_cast<int>(cinfo.image_height);
	m.comps = cinfo.num_components;
	m.cs    = cinfo.jpeg_color_space;
	m.prog  = cinfo.progressive_mode;

	for (std::size_t i = 0; i < static_cast<std::size_t>(cinfo.num_components); ++i) {
		m.h_samp[i] = cinfo.comp_info[i].h_samp_factor;
		m.v_samp[i] = cinfo.comp_info[i].v_samp_factor;
	}

	for (int i = 0; i < NUM_QUANT_TBLS; ++i)
		if (cinfo.quant_tbl_ptrs[i])
			++m.qt;
	for (int i = 0; i < NUM_HUFF_TBLS; ++i) {
		if (cinfo.dc_huff_tbl_ptrs[i])
			++m.hDC;
		if (cinfo.ac_huff_tbl_ptrs[i])
			++m.hAC;
	}

	const int APP1 = JPEG_APP0 + 1;
	for (jpeg_saved_marker_ptr mk = cinfo.marker_list; mk; mk = mk->next) {
		if (mk->marker == JPEG_APP0 && mk->data_length >= 5 && std::memcmp(mk->data, "JFIF", 5) == 0)
			m.jfif = true;
		if (mk->marker == APP1 && mk->data_length >= 6 && std::memcmp(mk->data, "Exif\0\0", 6) == 0)
			m.exif = true;
	}

	jpeg_destroy_decompress(&cinfo);
	fclose(fp);
	return true;
}

/*--------------------------- pretty printer -------------------------------*/
static const char* cs_name(J_COLOR_SPACE cs) {
	switch (cs) {
	case JCS_GRAYSCALE:
		return "Gray";
	case JCS_RGB:
		return "RGB";
	case JCS_YCbCr:
		return "YCbCr";
	case JCS_CMYK:
		return "CMYK";
	case JCS_YCCK:
		return "YCCK";
	default:
		return "Unknown";
	}
}
static std::string pad(int w, const std::string& s) {
	std::ostringstream o;
	o << std::left << std::setw(w) << s;
	return o.str();
}

/*─────────────────── Detect JPEG “process” type ────────────────────────────*/
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

/**
 * detect_jpeg_process
 *
 * Scans the JPEG file’s markers to determine its encoding “process”:
 *   • Baseline vs. Extended vs. Progressive vs. Lossless
 *   • Huffman vs. Arithmetic entropy coding
 *
 * Steps:
 * 1. Read and verify the SOI marker (0xFFD8). If absent, return Unknown.
 * 2. Iterate through markers until Start-Of-Scan (SOS, 0xFFDA) or End-Of-Image
 *    (EOI, 0xFFD9):
 *    a. Skip any 0xFF fill bytes, then read the marker byte.
 *    b. If it’s DHT (0xFFC4), note saw_dht = true.
 *       If it’s DAC (0xFFCC), note saw_dac = true.
 *    c. Read the two-byte segment length, then:
 *       – If this marker is one of the SOF0–SOF3 or SOF8–SOF11, immediately
 *         return the corresponding JpegProcess enum.
 *       – Otherwise, skip (length–2) bytes to move past this segment.
 * 3. If we exit on SOS or EOI without finding a SOF marker:
 *    – If any DHT tables were seen, assume Baseline Huffman.
 *    – Else if any DAC tables were seen, assume Baseline Arithmetic.
 *    – Otherwise return Unknown.
 */
static JpegProcess detect_jpeg_process(const fs::path& p) {
	std::ifstream in(p, std::ios::binary);
	if (!in)
		return JpegProcess::Unknown;

	// will flip if we encounter any Define Huffman Table or Define Arithmetic Coding
	bool saw_dht = false, saw_dac = false;

	unsigned char byte;
	unsigned char marker;

	// 1) skip SOI
	if (!in.read((char*)&byte, 1) || !in.read((char*)&byte, 1) || byte != 0xD8)
		return JpegProcess::Unknown;

	// 2) scan all markers until SOS (0xDA) or EOI (0xD9)
	while (true) {
		// find 0xFF
		do {
			if (!in.read((char*)&byte, 1))
				return JpegProcess::Unknown;
		} while (byte != 0xFF);

		// skip any repeat 0xFF fill bytes
		do {
			if (!in.read((char*)&marker, 1))
				return JpegProcess::Unknown;
		} while (marker == 0xFF);

		// if we hit Start-Of-Scan or End-Of-Image, stop
		if (marker == 0xDA || marker == 0xD9)
			break;

		// record DHT/DAC
		if (marker == 0xC4)
			saw_dht = true; // DHT
		else if (marker == 0xCC)
			saw_dac = true; // DAC

		// read segment length
		unsigned char hi, lo;
		if (!in.read((char*)&hi, 1) || !in.read((char*)&lo, 1))
			return JpegProcess::Unknown;
		unsigned length = (unsigned)hi << 8 | (unsigned)lo;

		// check for SOF markers
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
			// skip over this segment’s payload
			in.seekg(length - 2, std::ios::cur);
			break;
		}
	}

	// 3) no SOF seen → fall back on DHT/DAC
	if (saw_dht)
		return JpegProcess::Baseline_Huffman;
	if (saw_dac)
		return JpegProcess::Baseline_Arithmetic;
	return JpegProcess::Unknown;
}

static const char* to_string(JpegProcess p) {
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

/*─────────────────── side-by-side table with encoding ────────────────────*/
static void print_meta_table(const std::string& aName,
                             const JpegMeta&    A,
                             JpegProcess        pa,
                             const std::string& bName,
                             const JpegMeta&    B,
                             JpegProcess        pb) {
	constexpr int L = 22, C = 14;
	auto          row = [&](const std::string& lbl, const std::string& a, const std::string& b) {
        std::cout << pad(L, lbl) << " | " << pad(C, a) << " | " << pad(C, b) << "\n";
	};
	std::cout << "\n"
	          << pad(L, "") << " | " << pad(C, aName) << " | " << pad(C, bName) << "\n"
	          << std::string(L + 2 * C + 5, '-') << "\n";

	row("File size (B)", std::to_string(A.size), std::to_string(B.size));
	row("Dimensions", std::to_string(A.w) + 'x' + std::to_string(A.h), std::to_string(B.w) + 'x' + std::to_string(B.h));
	row("# components", std::to_string(A.comps), std::to_string(B.comps));
	row("Colour space", cs_name(A.cs), cs_name(B.cs));
	row("Progressive", A.prog ? "Yes" : "No", B.prog ? "Yes" : "No");
	auto samp = [](const JpegMeta& m) {
		std::ostringstream s;
		s << m.h_samp[0] << 'x' << m.v_samp[0];
		if (m.comps > 1)
			s << ", " << m.h_samp[1] << 'x' << m.v_samp[1];
		return s.str();
	};
	row("Sampling", samp(A), samp(B));
	row("Quant tables", std::to_string(A.qt), std::to_string(B.qt));
	row("Huff DC tbl", std::to_string(A.hDC), std::to_string(B.hDC));
	row("Huff AC tbl", std::to_string(A.hAC), std::to_string(B.hAC));
	row("JFIF", A.jfif ? "Yes" : "No", B.jfif ? "Yes" : "No");
	row("EXIF", A.exif ? "Yes" : "No", B.exif ? "Yes" : "No");
	row("Encoding", to_string(pa), to_string(pb));
	std::cout << std::string(L + 2 * C + 5, '-') << "\n";
}

/*==========================================================================
 * 2. ORIGINAL WORKFLOW (decode → recompress → PSNR) + META + PROCESS REPORT
 *==========================================================================*/
static bool process(const fs::path& src, const fs::path& outDir, std::ofstream& rep) {
	// load original
	std::uintmax_t srcSize = fs::file_size(src);
	int            w, h, ch;
	unsigned char* img = stbi_load(src.string().c_str(), &w, &h, &ch, 0);
	if (!img) {
		std::cerr << "Can't decode " << src << ": " << stbi_failure_reason() << "\n";
		return false;
	}
	std::size_t raw = static_cast<std::size_t>(w) * h * ch;
	fs::path    dst = outDir / (src.stem().string() + "_recompressed.jpg");

	// compress with libjpeg
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	unsigned char* outBuf = nullptr;
	unsigned long  outSz  = 0;
	jpeg_mem_dest(&cinfo, &outBuf, &outSz);
	cinfo.image_width      = static_cast<JDIMENSION>(w);
	cinfo.image_height     = static_cast<JDIMENSION>(h);
	cinfo.input_components = ch;
	cinfo.in_color_space   = (ch == 3 ? JCS_RGB : JCS_GRAYSCALE);
	jpeg_set_defaults(&cinfo);
	cinfo.optimize_coding = TRUE;
	jpeg_start_compress(&cinfo, TRUE);
	while (cinfo.next_scanline < cinfo.image_height) {
		JSAMPROW row = &img[cinfo.next_scanline * (std::size_t)w * ch];
		jpeg_write_scanlines(&cinfo, &row, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	// save to disk
	{
		std::ofstream fout(dst, std::ios::binary);
		fout.write(reinterpret_cast<char*>(outBuf), (std::streamsize)outSz);
	}
	std::uintmax_t dstSize = fs::file_size(dst);

	// reload recompressed
	int            w2, h2, ch2;
	unsigned char* img2 = stbi_load(dst.string().c_str(), &w2, &h2, &ch2, 0);

	// compute PSNR
	double psnr = 0;
	if (img2 && w == w2 && h == h2 && ch == ch2) {
		double mse = 0;
		for (std::size_t i = 0; i < raw; ++i) {
			double d = double(img[i]) - double(img2[i]);
			mse += d * d;
		}
		mse /= raw;
		psnr = 10. * std::log10((255. * 255.) / mse);
	}

	// collect metadata & process type
	JpegMeta A, B;
	inspect_jpeg(src, A);
	inspect_jpeg(dst, B);
	JpegProcess pa = detect_jpeg_process(src);
	JpegProcess pb = detect_jpeg_process(dst);

	// print table
	print_meta_table(src.filename().string(), A, pa, dst.filename().string(), B, pb);

	// write to text report (encoding now in table, so drop those lines)
	rep << "File: " << src.filename() << "\n"
	    << " - original size: " << srcSize << " B\n"
	    << " - recompressed:   " << dst.filename() << " (" << dstSize << " B)\n"
	    << " - compression ratio (raw/orig): " << double(raw) / double(srcSize) << "\n"
	    << " - PSNR: " << psnr << " dB\n"
	    << "First 10 bytes (orig): ";
	for (std::size_t i = 0; i < 10; ++i)
		rep << byte_bits(img[i]) << ((i + 1) % ch == 0 ? '\n' : ' ');
	rep << "\n--------------------------------------\n";

	// console echo
	std::cout << "Recompressed " << src.filename() << " -> " << dst.filename() << " (" << dstSize << " B)  PSNR "
	          << psnr << " dB\n";

	// cleanup
	stbi_image_free(img);
	if (img2)
		stbi_image_free(img2);
	std::free(outBuf);
	return true;
}

/*==========================================================================
 * 3. DRIVER
 *==========================================================================*/
int main() {
	fs::path root   = fastlanes::string {FLS_CMAKE_SOURCE_DIR};
	fs::path inDir  = root / "jpeg_playground/example/example_image";
	fs::path outDir = root / "jpeg_playground/tmp";
	fs::path rpt    = outDir / "jpeg_compression_report.txt";

	if (!fs::exists(outDir))
		fs::create_directories(outDir);
	std::ofstream report(rpt, std::ios::trunc);
	if (!report.is_open()) {
		std::cerr << "Can't open " << rpt << "\n";
		return 1;
	}

	int ok = 0, fail = 0;
	for (auto& e : fs::directory_iterator(inDir)) {
		if (!e.is_regular_file())
			continue;
		auto ext = to_lower(e.path().extension().string());
		if (ext == ".jpg" || ext == ".jpeg") {
			if (process(e.path(), outDir, report))
				++ok;
			else
				++fail;
		}
	}

	report << "Processed " << ok << ", Failed " << fail << "\n";
	std::cout << "Report written to " << rpt << "\n";
	return 0;
}
