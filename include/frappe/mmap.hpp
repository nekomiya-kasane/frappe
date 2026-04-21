#ifndef FRAPPE_MMAP_HPP
#define FRAPPE_MMAP_HPP

#include "frappe/error.hpp"
#include "frappe/exports.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

namespace frappe {

    using path = std::filesystem::path;

    // ============================================================================
    // Mapping modes
    // ============================================================================

    enum class map_mode {
        read_only,     // Read-only mapping
        read_write,    // Read-write mapping (changes written to file)
        copy_on_write, // Copy-on-write (changes not written to file)
        exec           // Executable mapping
    };

    // ============================================================================
    // Mapping advice (performance hints)
    // ============================================================================

    enum class map_advice {
        normal,     // Default access pattern
        sequential, // Sequential access
        random,     // Random access
        willneed,   // Will need soon (prefetch)
        dontneed    // Won't need soon
    };

    // ============================================================================
    // Memory-mapped file class
    // ============================================================================

    class FRAPPE_API mapped_file {
      public:
        // Constructors/Destructor
        mapped_file() noexcept;
        ~mapped_file();

        // Non-copyable, movable
        mapped_file(const mapped_file &) = delete;
        mapped_file &operator=(const mapped_file &) = delete;
        mapped_file(mapped_file &&other) noexcept;
        mapped_file &operator=(mapped_file &&other) noexcept;

        // Factory methods
        [[nodiscard]] static result<mapped_file> open(const path &p, map_mode mode = map_mode::read_only) noexcept;

        [[nodiscard]] static result<mapped_file> open(const path &p, std::size_t offset, std::size_t length,
                                                      map_mode mode = map_mode::read_only) noexcept;

        [[nodiscard]] static result<mapped_file> create(const path &p, std::size_t size,
                                                        map_mode mode = map_mode::read_write) noexcept;

        // Close mapping
        void close() noexcept;

        // State queries
        [[nodiscard]] bool is_open() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return is_open(); }
        [[nodiscard]] const path &file_path() const noexcept;
        [[nodiscard]] map_mode mode() const noexcept;

        // Data access
        [[nodiscard]] const std::byte *data() const noexcept;
        [[nodiscard]] std::byte *data() noexcept;
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        // Range access
        [[nodiscard]] const std::byte *begin() const noexcept { return data(); }
        [[nodiscard]] const std::byte *end() const noexcept { return data() + size(); }
        [[nodiscard]] std::byte *begin() noexcept { return data(); }
        [[nodiscard]] std::byte *end() noexcept { return data() + size(); }

        // Typed access
        template <typename T> [[nodiscard]] std::span<const T> as() const noexcept {
            auto *ptr = reinterpret_cast<const T *>(data());
            return std::span<const T>(ptr, size() / sizeof(T));
        }

        template <typename T> [[nodiscard]] std::span<T> as() noexcept {
            auto *ptr = reinterpret_cast<T *>(data());
            return std::span<T>(ptr, size() / sizeof(T));
        }

        template <typename T> [[nodiscard]] const T &at(std::size_t offset) const {
            if (offset + sizeof(T) > size()) {
                throw std::out_of_range("mapped_file::at: offset out of range");
            }
            return *reinterpret_cast<const T *>(data() + offset);
        }

        template <typename T> [[nodiscard]] T &at(std::size_t offset) {
            if (offset + sizeof(T) > size()) {
                throw std::out_of_range("mapped_file::at: offset out of range");
            }
            return *reinterpret_cast<T *>(data() + offset);
        }

        // String view (for text files)
        [[nodiscard]] std::string_view as_string() const noexcept {
            return std::string_view(reinterpret_cast<const char *>(data()), size());
        }

        // Performance hints
        void_result advise(map_advice advice) noexcept;
        void_result advise(std::size_t offset, std::size_t length, map_advice advice) noexcept;

        // Sync to disk
        void_result sync() noexcept;
        void_result sync(std::size_t offset, std::size_t length) noexcept;

        // Lock in memory (prevent swapping)
        void_result lock() noexcept;
        void_result unlock() noexcept;

        // Resize (read_write mode only)
        void_result resize(std::size_t new_size) noexcept;

      private:
        struct impl;
        std::unique_ptr<impl> _impl;

        mapped_file(std::unique_ptr<impl> impl) noexcept;
    };

    // ============================================================================
    // Convenience functions
    // ============================================================================

    [[nodiscard]] inline result<mapped_file> mmap_read(const path &p) noexcept {
        return mapped_file::open(p, map_mode::read_only);
    }

    [[nodiscard]] inline result<mapped_file> mmap_write(const path &p) noexcept {
        return mapped_file::open(p, map_mode::read_write);
    }

    [[nodiscard]] inline result<mapped_file> mmap_create(const path &p, std::size_t size) noexcept {
        return mapped_file::create(p, size, map_mode::read_write);
    }

    // ============================================================================
    // Mapped view (lightweight, non-owning view into mapped_file)
    // ============================================================================

    class FRAPPE_API mapped_view {
      public:
        mapped_view() noexcept = default;
        mapped_view(const mapped_file &file) noexcept;
        mapped_view(const mapped_file &file, std::size_t offset, std::size_t length) noexcept;
        mapped_view(const std::byte *data, std::size_t size) noexcept;

        [[nodiscard]] const std::byte *data() const noexcept { return _data; }
        [[nodiscard]] std::size_t size() const noexcept { return _size; }
        [[nodiscard]] bool empty() const noexcept { return _size == 0; }

        [[nodiscard]] const std::byte *begin() const noexcept { return _data; }
        [[nodiscard]] const std::byte *end() const noexcept { return _data + _size; }

        [[nodiscard]] std::string_view as_string() const noexcept {
            return std::string_view(reinterpret_cast<const char *>(_data), _size);
        }

        template <typename T> [[nodiscard]] std::span<const T> as() const noexcept {
            return std::span<const T>(reinterpret_cast<const T *>(_data), _size / sizeof(T));
        }

        [[nodiscard]] mapped_view subview(std::size_t offset, std::size_t length) const noexcept {
            if (offset >= _size) {
                return mapped_view{};
            }
            length = std::min(length, _size - offset);
            return mapped_view{_data + offset, length};
        }

        [[nodiscard]] std::byte operator[](std::size_t index) const noexcept { return _data[index]; }

      private:
        const std::byte *_data = nullptr;
        std::size_t _size = 0;
    };

    // ============================================================================
    // Chunked file mapping for large files
    // ============================================================================

    class FRAPPE_API mapped_file_chunks {
      public:
        explicit mapped_file_chunks(const path &p, std::size_t chunk_size = 64 * 1024 * 1024);

        class iterator {
          public:
            using iterator_category = std::input_iterator_tag;
            using value_type = mapped_view;
            using difference_type = std::ptrdiff_t;
            using pointer = const mapped_view *;
            using reference = const mapped_view &;

            iterator() = default;
            iterator(mapped_file_chunks *parent, std::size_t chunk_index);

            reference operator*() const { return _current_view; }
            pointer operator->() const { return &_current_view; }

            iterator &operator++();
            iterator operator++(int);

            bool operator==(const iterator &other) const;
            bool operator!=(const iterator &other) const { return !(*this == other); }

          private:
            mapped_file_chunks *_parent = nullptr;
            std::size_t _chunk_index = 0;
            mapped_view _current_view;

            void load_chunk();
        };

        [[nodiscard]] iterator begin();
        [[nodiscard]] iterator end();

        [[nodiscard]] std::size_t chunk_count() const noexcept;
        [[nodiscard]] std::size_t file_size() const noexcept;
        [[nodiscard]] std::size_t chunk_size() const noexcept { return _chunk_size; }

      private:
        path _path;
        std::size_t _file_size = 0;
        std::size_t _chunk_size;
        std::unique_ptr<mapped_file> _current_mapping;

        friend class iterator;
        void load_chunk(std::size_t index);
        [[nodiscard]] mapped_view get_chunk_view(std::size_t index) const;
    };

    // ============================================================================
    // Utility functions
    // ============================================================================

    // Prefetch file data into memory
    FRAPPE_API void_result prefetch(const path &p, std::size_t offset = 0, std::size_t length = 0) noexcept;

    // Check if pages are in memory (Linux only, returns empty on other platforms)
    [[nodiscard]] FRAPPE_API result<std::vector<bool>> mincore(const mapped_file &file) noexcept;

} // namespace frappe

#endif // FRAPPE_MMAP_HPP
