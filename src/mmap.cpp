#include "frappe/mmap.hpp"

#include <algorithm>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

namespace frappe {

    // ============================================================================
    // Platform-specific implementation
    // ============================================================================

    struct mapped_file::impl {
        path file_path;
        map_mode mode = map_mode::read_only;
        std::byte *data = nullptr;
        std::size_t size = 0;
        std::size_t offset = 0;

#ifdef _WIN32
        HANDLE file_handle = INVALID_HANDLE_VALUE;
        HANDLE mapping_handle = nullptr;
#else
        int fd = -1;
#endif

        ~impl() { unmap(); }

        void unmap() noexcept {
#ifdef _WIN32
            if (data) {
                UnmapViewOfFile(data);
                data = nullptr;
            }
            if (mapping_handle) {
                CloseHandle(mapping_handle);
                mapping_handle = nullptr;
            }
            if (file_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(file_handle);
                file_handle = INVALID_HANDLE_VALUE;
            }
#else
            if (data && data != MAP_FAILED) {
                munmap(data, size);
                data = nullptr;
            }
            if (fd >= 0) {
                ::close(fd);
                fd = -1;
            }
#endif
            size = 0;
        }
    };

    // ============================================================================
    // mapped_file implementation
    // ============================================================================

    mapped_file::mapped_file() noexcept = default;

    mapped_file::mapped_file(std::unique_ptr<impl> impl) noexcept : _impl(std::move(impl)) {}

    mapped_file::~mapped_file() = default;

    mapped_file::mapped_file(mapped_file &&other) noexcept : _impl(std::move(other._impl)) {}

    mapped_file &mapped_file::operator=(mapped_file &&other) noexcept {
        if (this != &other) {
            _impl = std::move(other._impl);
        }
        return *this;
    }

    result<mapped_file> mapped_file::open(const path &p, map_mode mode) noexcept {
        std::error_code ec;
        auto file_size = std::filesystem::file_size(p, ec);
        if (ec) {
            return std::unexpected(ec);
        }
        return open(p, 0, static_cast<std::size_t>(file_size), mode);
    }

    result<mapped_file> mapped_file::open(const path &p, std::size_t offset, std::size_t length,
                                          map_mode mode) noexcept {
        auto pimpl = std::make_unique<impl>();
        pimpl->file_path = p;
        pimpl->mode = mode;
        pimpl->offset = offset;

#ifdef _WIN32
        // Windows implementation
        DWORD access = 0;
        DWORD share = FILE_SHARE_READ;
        DWORD protect = 0;
        DWORD map_access = 0;

        switch (mode) {
        case map_mode::read_only:
            access = GENERIC_READ;
            protect = PAGE_READONLY;
            map_access = FILE_MAP_READ;
            break;
        case map_mode::read_write:
            access = GENERIC_READ | GENERIC_WRITE;
            share = 0;
            protect = PAGE_READWRITE;
            map_access = FILE_MAP_ALL_ACCESS;
            break;
        case map_mode::copy_on_write:
            access = GENERIC_READ;
            protect = PAGE_WRITECOPY;
            map_access = FILE_MAP_COPY;
            break;
        case map_mode::exec:
            access = GENERIC_READ | GENERIC_EXECUTE;
            protect = PAGE_EXECUTE_READ;
            map_access = FILE_MAP_READ | FILE_MAP_EXECUTE;
            break;
        }

        pimpl->file_handle =
            CreateFileW(p.c_str(), access, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (pimpl->file_handle == INVALID_HANDLE_VALUE) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(pimpl->file_handle, &file_size)) {
            auto err = GetLastError();
            CloseHandle(pimpl->file_handle);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        if (length == 0) {
            length = static_cast<std::size_t>(file_size.QuadPart) - offset;
        }

        pimpl->mapping_handle = CreateFileMappingW(pimpl->file_handle, nullptr, protect, 0, 0, nullptr);

        if (!pimpl->mapping_handle) {
            auto err = GetLastError();
            CloseHandle(pimpl->file_handle);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        DWORD offset_high = static_cast<DWORD>(offset >> 32);
        DWORD offset_low = static_cast<DWORD>(offset & 0xFFFFFFFF);

        pimpl->data =
            static_cast<std::byte *>(MapViewOfFile(pimpl->mapping_handle, map_access, offset_high, offset_low, length));

        if (!pimpl->data) {
            auto err = GetLastError();
            CloseHandle(pimpl->mapping_handle);
            CloseHandle(pimpl->file_handle);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        pimpl->size = length;

#else
        // POSIX implementation
        int flags = 0;
        int prot = 0;

        switch (mode) {
        case map_mode::read_only:
            flags = O_RDONLY;
            prot = PROT_READ;
            break;
        case map_mode::read_write:
            flags = O_RDWR;
            prot = PROT_READ | PROT_WRITE;
            break;
        case map_mode::copy_on_write:
            flags = O_RDONLY;
            prot = PROT_READ | PROT_WRITE;
            break;
        case map_mode::exec:
            flags = O_RDONLY;
            prot = PROT_READ | PROT_EXEC;
            break;
        }

        pimpl->fd = ::open(p.c_str(), flags);
        if (pimpl->fd < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        struct stat st;
        if (fstat(pimpl->fd, &st) < 0) {
            auto err = errno;
            ::close(pimpl->fd);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        if (length == 0) {
            length = static_cast<std::size_t>(st.st_size) - offset;
        }

        int map_flags = (mode == map_mode::copy_on_write) ? MAP_PRIVATE : MAP_SHARED;

        void *addr = mmap(nullptr, length, prot, map_flags, pimpl->fd, static_cast<off_t>(offset));
        if (addr == MAP_FAILED) {
            auto err = errno;
            ::close(pimpl->fd);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        pimpl->data = static_cast<std::byte *>(addr);
        pimpl->size = length;
#endif

        return mapped_file(std::move(pimpl));
    }

    result<mapped_file> mapped_file::create(const path &p, std::size_t size, map_mode mode) noexcept {
        auto pimpl = std::make_unique<impl>();
        pimpl->file_path = p;
        pimpl->mode = mode;

#ifdef _WIN32
        DWORD access = GENERIC_READ | GENERIC_WRITE;
        DWORD protect = PAGE_READWRITE;
        DWORD map_access = FILE_MAP_ALL_ACCESS;

        pimpl->file_handle = CreateFileW(p.c_str(), access, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (pimpl->file_handle == INVALID_HANDLE_VALUE) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }

        LARGE_INTEGER li;
        li.QuadPart = static_cast<LONGLONG>(size);

        pimpl->mapping_handle =
            CreateFileMappingW(pimpl->file_handle, nullptr, protect, li.HighPart, li.LowPart, nullptr);

        if (!pimpl->mapping_handle) {
            auto err = GetLastError();
            CloseHandle(pimpl->file_handle);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        pimpl->data = static_cast<std::byte *>(MapViewOfFile(pimpl->mapping_handle, map_access, 0, 0, size));

        if (!pimpl->data) {
            auto err = GetLastError();
            CloseHandle(pimpl->mapping_handle);
            CloseHandle(pimpl->file_handle);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        pimpl->size = size;

#else
        pimpl->fd = ::open(p.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (pimpl->fd < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        if (ftruncate(pimpl->fd, static_cast<off_t>(size)) < 0) {
            auto err = errno;
            ::close(pimpl->fd);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        void *addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, pimpl->fd, 0);
        if (addr == MAP_FAILED) {
            auto err = errno;
            ::close(pimpl->fd);
            return std::unexpected(std::error_code(err, std::system_category()));
        }

        pimpl->data = static_cast<std::byte *>(addr);
        pimpl->size = size;
#endif

        return mapped_file(std::move(pimpl));
    }

    void mapped_file::close() noexcept {
        if (_impl) {
            _impl->unmap();
        }
    }

    bool mapped_file::is_open() const noexcept {
        return _impl && _impl->data != nullptr;
    }

    const path &mapped_file::file_path() const noexcept {
        static const path empty_path;
        return _impl ? _impl->file_path : empty_path;
    }

    map_mode mapped_file::mode() const noexcept {
        return _impl ? _impl->mode : map_mode::read_only;
    }

    const std::byte *mapped_file::data() const noexcept {
        return _impl ? _impl->data : nullptr;
    }

    std::byte *mapped_file::data() noexcept {
        return _impl ? _impl->data : nullptr;
    }

    std::size_t mapped_file::size() const noexcept {
        return _impl ? _impl->size : 0;
    }

    void_result mapped_file::advise(map_advice advice) noexcept {
        return advise(0, size(), advice);
    }

    void_result mapped_file::advise(std::size_t offset, std::size_t length, map_advice advice) noexcept {
        if (!is_open()) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

#ifdef _WIN32
        // Windows doesn't have madvise, but we can use PrefetchVirtualMemory for willneed
        if (advice == map_advice::willneed) {
            WIN32_MEMORY_RANGE_ENTRY entry;
            entry.VirtualAddress = _impl->data + offset;
            entry.NumberOfBytes = length;
            PrefetchVirtualMemory(GetCurrentProcess(), 1, &entry, 0);
        }
        return {};
#else
        int adv = MADV_NORMAL;
        switch (advice) {
        case map_advice::normal:
            adv = MADV_NORMAL;
            break;
        case map_advice::sequential:
            adv = MADV_SEQUENTIAL;
            break;
        case map_advice::random:
            adv = MADV_RANDOM;
            break;
        case map_advice::willneed:
            adv = MADV_WILLNEED;
            break;
        case map_advice::dontneed:
            adv = MADV_DONTNEED;
            break;
        }

        if (madvise(_impl->data + offset, length, adv) < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        return {};
#endif
    }

    void_result mapped_file::sync() noexcept {
        return sync(0, size());
    }

    void_result mapped_file::sync(std::size_t offset, std::size_t length) noexcept {
        if (!is_open()) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

#ifdef _WIN32
        if (!FlushViewOfFile(_impl->data + offset, length)) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }
        if (!FlushFileBuffers(_impl->file_handle)) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }
        return {};
#else
        if (msync(_impl->data + offset, length, MS_SYNC) < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        return {};
#endif
    }

    void_result mapped_file::lock() noexcept {
        if (!is_open()) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

#ifdef _WIN32
        if (!VirtualLock(_impl->data, _impl->size)) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }
        return {};
#else
        if (mlock(_impl->data, _impl->size) < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        return {};
#endif
    }

    void_result mapped_file::unlock() noexcept {
        if (!is_open()) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

#ifdef _WIN32
        if (!VirtualUnlock(_impl->data, _impl->size)) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }
        return {};
#else
        if (munlock(_impl->data, _impl->size) < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }
        return {};
#endif
    }

    void_result mapped_file::resize(std::size_t new_size) noexcept {
        if (!is_open()) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

        if (_impl->mode != map_mode::read_write) {
            return std::unexpected(std::make_error_code(std::errc::operation_not_permitted));
        }

        // Close current mapping and reopen with new size
        auto p = _impl->file_path;
        auto m = _impl->mode;

#ifdef _WIN32
        if (_impl->data) {
            UnmapViewOfFile(_impl->data);
            _impl->data = nullptr;
        }
        if (_impl->mapping_handle) {
            CloseHandle(_impl->mapping_handle);
            _impl->mapping_handle = nullptr;
        }

        LARGE_INTEGER li;
        li.QuadPart = static_cast<LONGLONG>(new_size);
        if (!SetFilePointerEx(_impl->file_handle, li, nullptr, FILE_BEGIN)) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }
        if (!SetEndOfFile(_impl->file_handle)) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }

        _impl->mapping_handle =
            CreateFileMappingW(_impl->file_handle, nullptr, PAGE_READWRITE, li.HighPart, li.LowPart, nullptr);

        if (!_impl->mapping_handle) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }

        _impl->data =
            static_cast<std::byte *>(MapViewOfFile(_impl->mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0, new_size));

        if (!_impl->data) {
            return std::unexpected(std::error_code(GetLastError(), std::system_category()));
        }

        _impl->size = new_size;
        return {};
#else
        // Try mremap first (Linux-specific)
#    ifdef __linux__
        void *new_addr = mremap(_impl->data, _impl->size, new_size, MREMAP_MAYMOVE);
        if (new_addr != MAP_FAILED) {
            _impl->data = static_cast<std::byte *>(new_addr);
            _impl->size = new_size;

            if (ftruncate(_impl->fd, static_cast<off_t>(new_size)) < 0) {
                return std::unexpected(std::error_code(errno, std::system_category()));
            }
            return {};
        }
#    endif

        // Fallback: unmap, resize file, remap
        munmap(_impl->data, _impl->size);

        if (ftruncate(_impl->fd, static_cast<off_t>(new_size)) < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        void *addr = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, _impl->fd, 0);
        if (addr == MAP_FAILED) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        _impl->data = static_cast<std::byte *>(addr);
        _impl->size = new_size;
        return {};
#endif
    }

    // ============================================================================
    // mapped_view implementation
    // ============================================================================

    mapped_view::mapped_view(const mapped_file &file) noexcept : _data(file.data()), _size(file.size()) {}

    mapped_view::mapped_view(const mapped_file &file, std::size_t offset, std::size_t length) noexcept {
        if (offset < file.size()) {
            _data = file.data() + offset;
            _size = std::min(length, file.size() - offset);
        }
    }

    mapped_view::mapped_view(const std::byte *data, std::size_t size) noexcept : _data(data), _size(size) {}

    // ============================================================================
    // mapped_file_chunks implementation
    // ============================================================================

    mapped_file_chunks::mapped_file_chunks(const path &p, std::size_t chunk_size) : _path(p), _chunk_size(chunk_size) {
        std::error_code ec;
        _file_size = static_cast<std::size_t>(std::filesystem::file_size(p, ec));
    }

    std::size_t mapped_file_chunks::chunk_count() const noexcept {
        if (_file_size == 0) {
            return 0;
        }
        return (_file_size + _chunk_size - 1) / _chunk_size;
    }

    std::size_t mapped_file_chunks::file_size() const noexcept {
        return _file_size;
    }

    void mapped_file_chunks::load_chunk(std::size_t index) {
        std::size_t offset = index * _chunk_size;
        std::size_t length = std::min(_chunk_size, _file_size - offset);

        auto result = mapped_file::open(_path, offset, length, map_mode::read_only);
        if (result) {
            _current_mapping = std::make_unique<mapped_file>(std::move(*result));
        }
    }

    mapped_view mapped_file_chunks::get_chunk_view(std::size_t index) const {
        if (_current_mapping && _current_mapping->is_open()) {
            return mapped_view(*_current_mapping);
        }
        return mapped_view{};
    }

    mapped_file_chunks::iterator::iterator(mapped_file_chunks *parent, std::size_t chunk_index)
        : _parent(parent), _chunk_index(chunk_index) {
        if (_parent && _chunk_index < _parent->chunk_count()) {
            load_chunk();
        }
    }

    void mapped_file_chunks::iterator::load_chunk() {
        if (_parent && _chunk_index < _parent->chunk_count()) {
            _parent->load_chunk(_chunk_index);
            _current_view = _parent->get_chunk_view(_chunk_index);
        }
    }

    mapped_file_chunks::iterator &mapped_file_chunks::iterator::operator++() {
        ++_chunk_index;
        if (_parent && _chunk_index < _parent->chunk_count()) {
            load_chunk();
        }
        return *this;
    }

    mapped_file_chunks::iterator mapped_file_chunks::iterator::operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    bool mapped_file_chunks::iterator::operator==(const iterator &other) const {
        return _parent == other._parent && _chunk_index == other._chunk_index;
    }

    mapped_file_chunks::iterator mapped_file_chunks::begin() {
        return iterator(this, 0);
    }

    mapped_file_chunks::iterator mapped_file_chunks::end() {
        return iterator(this, chunk_count());
    }

    // ============================================================================
    // Utility functions
    // ============================================================================

    void_result prefetch(const path &p, std::size_t offset, std::size_t length) noexcept {
        auto result = mapped_file::open(p, map_mode::read_only);
        if (!result) {
            return std::unexpected(result.error());
        }

        if (length == 0) {
            length = result->size();
        }

        return result->advise(offset, length, map_advice::willneed);
    }

    result<std::vector<bool>> mincore(const mapped_file &file) noexcept {
#ifdef __linux__
        if (!file.is_open()) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

        std::size_t page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
        std::size_t num_pages = (file.size() + page_size - 1) / page_size;

        std::vector<unsigned char> vec(num_pages);
        if (::mincore(const_cast<std::byte *>(file.data()), file.size(), vec.data()) < 0) {
            return std::unexpected(std::error_code(errno, std::system_category()));
        }

        std::vector<bool> result(num_pages);
        for (std::size_t i = 0; i < num_pages; ++i) {
            result[i] = (vec[i] & 1) != 0;
        }
        return result;
#else
        // Not supported on Windows/macOS
        return std::vector<bool>{};
#endif
    }

} // namespace frappe
