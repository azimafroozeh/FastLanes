// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/io/file.cpp
// ────────────────────────────────────────────────────────
#include "fls/io/file.hpp"
#include "fls/common/alias.hpp"
#include "fls/common/assert.hpp"
#include "fls/cor/lyt/buf.hpp"
#include "fls/std/filesystem.hpp"
#include "fls/std/string.hpp"
#include <cerrno>  // for errno, EINTR
#include <cstdint> // for int64_t
#include <fstream> // for std::ifstream, std::ofstream
#include <ios>     // for std::ios, std::streamoff, std::streamsize
#include <memory>  // for std::make_unique
#include <sstream>
#include <unistd.h>   // for ::pread, ::close

#if defined(__APPLE__)
#include <sys/_types/_off_t.h>
#include <sys/_types/_ssize_t.h>
#elif defined(__linux__)
#include <sys/types.h> // for ssize_t, off_t
#endif

namespace fastlanes {

File::File(const path& path) // NOLINT
    : m_path(path) {
}

File::~File() {

	if (m_of_stream != nullptr) {
		FileSystem::close(*m_of_stream);
	}

	if (m_fd >= 0) {
		FileSystem::close(m_fd);
	}
}

void File::Write(const Buf& buf) {
	if (m_of_stream == nullptr) {
		m_of_stream = make_unique<std::ofstream>(FileSystem::open_w(m_path));
	}
	//
	m_of_stream->write(reinterpret_cast<char*>(buf.data()), static_cast<int64_t>(buf.Size()));
}

void File::Read(const Buf& buf) {
	if (m_fd == -1) {
		m_fd = FileSystem::open_r_binary(m_path);
	}
	if (m_file_size == -1) {
		m_file_size = FileSystem::read_file_size(m_fd);
	}

	FLS_ASSERT_LE(m_file_size, buf.Capacity());

	ReadInternal(buf.mutable_data(), static_cast<n_t>(m_file_size));
}

void File::ReadRange(const Buf& buf, const n_t offset, const n_t size) {
	if (m_fd == -1) {
		m_fd = FileSystem::open_r_binary(m_path);
	}
	if (m_file_size == -1) {
		m_file_size = FileSystem::read_file_size(m_fd);
	}

	FLS_ASSERT_LE(offset + size, m_file_size);
	FLS_ASSERT_LE(size, buf.Capacity());

	ReadInternal(buf.mutable_data(), size, static_cast<off_t>(offset));
}

n_t File::Size() {
	if (m_file_size == -1) {
		m_file_size = FileSystem::read_file_size(m_fd);
	}
	return static_cast<n_t>(m_file_size);
}

void File::Append(const Buf& buf) {
	if (m_of_stream == nullptr) {
		// Open file in append mode
		m_of_stream = std::make_unique<std::ofstream>(m_path, std::ios::binary | std::ios::app);
	}
	m_of_stream->write(reinterpret_cast<char*>(buf.data()), static_cast<int64_t>(buf.Size()));
}

void File::Append(const char* pointer, n_t size) {
	if (m_of_stream == nullptr) {
		// Open file in append mode
		m_of_stream = std::make_unique<std::ofstream>(m_path, std::ios::binary | std::ios::app);
	}
	m_of_stream->write(pointer, static_cast<int64_t>(size));
}

void File::ReadInternal(uint8_t* dst, const n_t size, const off_t offset) const {
	n_t done = 0;
	while (done < size) {
		const ssize_t n_bytes = ::pread(m_fd, dst + done, size - done, offset + static_cast<off_t>(done));
		if (n_bytes < 0 && errno == EINTR) {
			continue;
		}
		if (n_bytes <= 0) {
			FLS_ABORT("Could not read from file in read-only mode")
		}

		done += static_cast<n_t>(n_bytes);
	}
}

/*--------------------------------------------------------------------------------------------------------------------*\
 * STATIC
\*--------------------------------------------------------------------------------------------------------------------*/
string File::read(const path& file_path) {
	std::ifstream     json_stream = FileSystem::open_r(file_path);
	std::stringstream buffer;
	buffer << json_stream.rdbuf();
	return buffer.str();
}

void File::write(const path& dir_path, const string& dump) {
	auto file = FileSystem::open_w(dir_path);

	file << dump;

	FileSystem::close(file);
}

void File::append(const path& dir_path, const string& dump) {
	auto file = FileSystem::opend_app(dir_path);

	file << dump;

	FileSystem::close(file);
}
} // namespace fastlanes
