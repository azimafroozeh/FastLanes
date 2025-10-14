// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/include/fls/io/file.hpp
// ────────────────────────────────────────────────────────
#ifndef FLS_IO_FILE_HPP
#define FLS_IO_FILE_HPP

#include "fls/common/common.hpp"
#include "fls/std/filesystem.hpp"
#include "fls/std/string.hpp"

namespace fastlanes {
/*--------------------------------------------------------------------------------------------------------------------*/
class Buf;
/*--------------------------------------------------------------------------------------------------------------------*/

class File {
public:
	explicit File(const path& path);
	~File();

	File(const File&)            = delete;
	File& operator=(const File&) = delete;

public:
	// write to file_path
	void Write(const Buf& buf);
	// write to file_path
	void Read(const Buf& buf) const;
	// write to file_path
	void Append(const Buf& buf);
	// Append
	void Append(const char* pointer, n_t size);
	//
	void ReadRange(const Buf& buf, n_t offset, n_t size) const;
	// get file size
	[[nodiscard]] n_t Size() const;

public:
	/// read from file_path and return string.
	static string read(const path& file_path);
	/// write to file_path
	static void write(const path& file_path, const string& dump);
	/// append to file_path
	static void append(const path& file_path, const string& dump);

private:
	void ReadInternal(uint8_t* dst, n_t size, off_t offset = 0) const;

private:
	path              m_path;
	up<std::ofstream> m_of_stream;
	int               m_fd {-1};
	size_t            m_file_size {0};
};
} // namespace fastlanes

#endif // FLS_IO_FILE_HPP
