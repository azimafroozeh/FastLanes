#ifndef FLS_STD_SPAN_HPP
#define FLS_STD_SPAN_HPP

// Prefer the real C++20 header if the toolchain has it
#if __has_include(<span>)
#include <span>
#endif

// If <span> is missing—or present but still no std::span—fall back
#ifndef __cpp_lib_span
#include <flatbuffers/stl_emulation.h> // flatbuffers::span poly-fill
namespace fastlanes {
using flatbuffers::span;
}
#else
namespace fastlanes {
using std::span;
}
#endif
