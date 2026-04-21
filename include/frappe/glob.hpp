#ifndef FRAPPE_GLOB_HPP
#define FRAPPE_GLOB_HPP

#include "frappe/exports.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace frappe {

    using path = std::filesystem::path;

    [[nodiscard]] FRAPPE_API std::vector<path> glob(std::string_view pattern);
    [[nodiscard]] FRAPPE_API std::vector<path> glob(const path &base_path, std::string_view pattern);

    [[nodiscard]] FRAPPE_API bool match(const path &p, std::string_view pattern);
    [[nodiscard]] FRAPPE_API bool fnmatch(std::string_view name, std::string_view pattern);

    class FRAPPE_API glob_iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = path;
        using difference_type = std::ptrdiff_t;
        using pointer = const path *;
        using reference = const path &;

        glob_iterator() = default;
        glob_iterator(const path &base_path, std::string_view pattern);

        reference operator*() const;
        pointer operator->() const;
        glob_iterator &operator++();
        glob_iterator operator++(int);

        bool operator==(const glob_iterator &other) const;
        bool operator!=(const glob_iterator &other) const;

      private:
        class impl;
        std::shared_ptr<impl> _impl;
    };

    class FRAPPE_API glob_range {
      public:
        glob_range(const path &base_path, std::string_view pattern);
        glob_range(std::string_view pattern);

        [[nodiscard]] glob_iterator begin() const;
        [[nodiscard]] glob_iterator end() const;

      private:
        path _base_path;
        std::string _pattern;
    };

    [[nodiscard]] inline glob_range iglob(std::string_view pattern) {
        return glob_range(pattern);
    }

    [[nodiscard]] inline glob_range iglob(const path &base_path, std::string_view pattern) {
        return glob_range(base_path, pattern);
    }

} // namespace frappe

#endif // FRAPPE_GLOB_HPP
