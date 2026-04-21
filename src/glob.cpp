#include "frappe/glob.hpp"

#include "frappe/error.hpp"

#include <algorithm>
#include <regex>
#include <stack>

namespace frappe {

    namespace {

        bool match_char(char c, char pattern, bool case_sensitive = true) {
            if (case_sensitive) {
                return c == pattern;
            }
            return std::tolower(static_cast<unsigned char>(c)) == std::tolower(static_cast<unsigned char>(pattern));
        }

        bool match_bracket(std::string_view &pattern, char c) {
            if (pattern.empty() || pattern[0] != '[') {
                return false;
            }

            pattern.remove_prefix(1);
            bool negate = false;
            if (!pattern.empty() && (pattern[0] == '!' || pattern[0] == '^')) {
                negate = true;
                pattern.remove_prefix(1);
            }

            bool matched = false;
            char prev_char = 0;
            bool first = true;

            while (!pattern.empty() && (first || pattern[0] != ']')) {
                first = false;

                if (pattern.size() >= 3 && pattern[1] == '-' && pattern[2] != ']') {
                    char range_start = pattern[0];
                    char range_end = pattern[2];
                    if (c >= range_start && c <= range_end) {
                        matched = true;
                    }
                    pattern.remove_prefix(3);
                    prev_char = range_end;
                } else {
                    if (pattern[0] == c) {
                        matched = true;
                    }
                    prev_char = pattern[0];
                    pattern.remove_prefix(1);
                }
            }

            if (!pattern.empty() && pattern[0] == ']') {
                pattern.remove_prefix(1);
            }

            return negate ? !matched : matched;
        }

        bool fnmatch_impl(std::string_view name, std::string_view pattern) {
            size_t n = 0;
            size_t p = 0;
            size_t star_p = std::string_view::npos;
            size_t star_n = 0;

            while (n < name.size()) {
                if (p < pattern.size() && pattern[p] == '*') {
                    star_p = p++;
                    star_n = n;
                } else if (p < pattern.size() && pattern[p] == '?') {
                    ++p;
                    ++n;
                } else if (p < pattern.size() && pattern[p] == '[') {
                    std::string_view bracket_pattern = pattern.substr(p);
                    if (match_bracket(bracket_pattern, name[n])) {
                        p = pattern.size() - bracket_pattern.size();
                        ++n;
                    } else if (star_p != std::string_view::npos) {
                        p = star_p + 1;
                        n = ++star_n;
                    } else {
                        return false;
                    }
                } else if (p < pattern.size() && name[n] == pattern[p]) {
                    ++p;
                    ++n;
                } else if (star_p != std::string_view::npos) {
                    p = star_p + 1;
                    n = ++star_n;
                } else {
                    return false;
                }
            }

            while (p < pattern.size() && pattern[p] == '*') {
                ++p;
            }

            return p == pattern.size();
        }

        bool is_recursive_pattern(std::string_view pattern) {
            return pattern.find("**") != std::string_view::npos;
        }

        std::pair<path, std::string> split_pattern(std::string_view pattern) {
            std::string pattern_str(pattern);

#ifdef _WIN32
            std::replace(pattern_str.begin(), pattern_str.end(), '/', '\\');
#endif

            path p(pattern_str);

            for (auto it = p.begin(); it != p.end(); ++it) {
                std::string component = it->string();
                if (component.find_first_of("*?[{") != std::string::npos) {
                    path base;
                    for (auto base_it = p.begin(); base_it != it; ++base_it) {
                        base /= *base_it;
                    }

                    path remaining;
                    for (auto rem_it = it; rem_it != p.end(); ++rem_it) {
                        remaining /= *rem_it;
                    }

                    return {base.empty() ? "." : base, remaining.string()};
                }
            }

            return {p.parent_path().empty() ? "." : p.parent_path(), p.filename().string()};
        }

        void glob_impl(const path &base_path, std::string_view pattern, std::vector<path> &results,
                       bool recursive = false) {

            if (pattern.empty()) {
                if (std::filesystem::exists(base_path)) {
                    results.push_back(base_path);
                }
                return;
            }

            std::string pattern_str(pattern);
            size_t sep_pos = pattern_str.find_first_of("/\\");
            std::string current_pattern;
            std::string remaining_pattern;

            if (sep_pos != std::string::npos) {
                current_pattern = pattern_str.substr(0, sep_pos);
                remaining_pattern = pattern_str.substr(sep_pos + 1);
            } else {
                current_pattern = pattern_str;
            }

            if (current_pattern == "**") {
                glob_impl(base_path, remaining_pattern, results, false);

                std::error_code ec;
                for (const auto &entry : std::filesystem::directory_iterator(base_path, ec)) {
                    if (entry.is_directory()) {
                        glob_impl(entry.path(), pattern, results, true);
                    }
                }
                return;
            }

            std::error_code ec;
            for (const auto &entry : std::filesystem::directory_iterator(base_path, ec)) {
                std::string filename = entry.path().filename().string();

                if (fnmatch_impl(filename, current_pattern)) {
                    if (remaining_pattern.empty()) {
                        results.push_back(entry.path());
                    } else if (entry.is_directory()) {
                        glob_impl(entry.path(), remaining_pattern, results, false);
                    }
                }
            }
        }

    } // anonymous namespace

    bool fnmatch(std::string_view name, std::string_view pattern) {
        return fnmatch_impl(name, pattern);
    }

    bool match(const path &p, std::string_view pattern) {
        return fnmatch_impl(p.filename().string(), pattern);
    }

    std::vector<path> glob(std::string_view pattern) {
        auto [base, pat] = split_pattern(pattern);
        return glob(base, pat);
    }

    std::vector<path> glob(const path &base_path, std::string_view pattern) {
        std::vector<path> results;

        if (!std::filesystem::exists(base_path)) {
            return results;
        }

        glob_impl(base_path, pattern, results);

        std::sort(results.begin(), results.end());
        return results;
    }

    class glob_iterator::impl {
      public:
        impl(const path &base_path, std::string_view pattern) : _results(glob(base_path, pattern)), _index(0) {}

        bool at_end() const { return _index >= _results.size(); }
        const path &current() const { return _results[_index]; }
        void advance() { ++_index; }

        bool equal(const impl *other) const {
            if (!other) {
                return at_end();
            }
            return _index == other->_index && _results.size() == other->_results.size();
        }

      private:
        std::vector<path> _results;
        size_t _index;
    };

    glob_iterator::glob_iterator(const path &base_path, std::string_view pattern)
        : _impl(std::make_shared<impl>(base_path, pattern)) {
        if (_impl->at_end()) {
            _impl.reset();
        }
    }

    glob_iterator::reference glob_iterator::operator*() const {
        return _impl->current();
    }

    glob_iterator::pointer glob_iterator::operator->() const {
        return &_impl->current();
    }

    glob_iterator &glob_iterator::operator++() {
        if (_impl) {
            _impl->advance();
            if (_impl->at_end()) {
                _impl.reset();
            }
        }
        return *this;
    }

    glob_iterator glob_iterator::operator++(int) {
        glob_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool glob_iterator::operator==(const glob_iterator &other) const {
        if (!_impl && !other._impl) {
            return true;
        }
        if (!_impl || !other._impl) {
            return false;
        }
        return _impl->equal(other._impl.get());
    }

    bool glob_iterator::operator!=(const glob_iterator &other) const {
        return !(*this == other);
    }

    glob_range::glob_range(const path &base_path, std::string_view pattern)
        : _base_path(base_path), _pattern(pattern) {}

    glob_range::glob_range(std::string_view pattern) : _base_path("."), _pattern(pattern) {
        auto [base, pat] = split_pattern(pattern);
        _base_path = base;
        _pattern = pat;
    }

    glob_iterator glob_range::begin() const {
        return glob_iterator(_base_path, _pattern);
    }

    glob_iterator glob_range::end() const {
        return glob_iterator();
    }

} // namespace frappe
