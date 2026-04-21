#include "frappe/find.hpp"

#include "frappe/glob.hpp"
#include "frappe/regex.hpp"

#include <algorithm>
#include <set>
#include <stack>

namespace frappe {

    find_options &find_options::name(std::string_view pattern) {
        std::string pat(pattern);
        _conditions.push_back(
            {[pat](const std::filesystem::directory_entry &e) { return fnmatch(e.path().filename().string(), pat); }});
        return *this;
    }

    find_options &find_options::iname(std::string_view pattern) {
        std::string pat(pattern);
        _conditions.push_back({[pat](const std::filesystem::directory_entry &e) {
            std::string filename = e.path().filename().string();
            std::string pattern_lower = pat;
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
            std::transform(pattern_lower.begin(), pattern_lower.end(), pattern_lower.begin(), ::tolower);
            return fnmatch(filename, pattern_lower);
        }});
        return *this;
    }

    find_options &find_options::regex(std::string_view pattern) {
        auto re = std::make_shared<compiled_regex>(pattern);
        _conditions.push_back(
            {[re](const std::filesystem::directory_entry &e) { return re->match(e.path().filename().string()); }});
        return *this;
    }

    find_options &find_options::iregex(std::string_view pattern) {
        auto re = std::make_shared<compiled_regex>(pattern, regex_options::caseless);
        _conditions.push_back(
            {[re](const std::filesystem::directory_entry &e) { return re->match(e.path().filename().string()); }});
        return *this;
    }

    find_options &find_options::path_regex(std::string_view pattern) {
        auto re = std::make_shared<compiled_regex>(pattern);
        _conditions.push_back(
            {[re](const std::filesystem::directory_entry &e) { return re->match(e.path().string()); }});
        return *this;
    }

    find_options &find_options::extension(std::string_view ext) {
        std::string extension_str(ext);
        if (!extension_str.empty() && extension_str[0] != '.') {
            extension_str = "." + extension_str;
        }
        _conditions.push_back({[extension_str](const std::filesystem::directory_entry &e) {
            return e.path().extension() == extension_str;
        }});
        return *this;
    }

    find_options &find_options::extensions(std::initializer_list<std::string_view> exts) {
        std::set<std::string> ext_set;
        for (auto ext : exts) {
            std::string s(ext);
            if (!s.empty() && s[0] != '.') {
                s = "." + s;
            }
            ext_set.insert(s);
        }
        _conditions.push_back({[ext_set](const std::filesystem::directory_entry &e) {
            return ext_set.contains(e.path().extension().string());
        }});
        return *this;
    }

    find_options &find_options::type(std::filesystem::file_type ft) {
        _conditions.push_back({[ft](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.status(ec).type() == ft;
        }});
        return *this;
    }

    find_options &find_options::is_file() {
        return type(std::filesystem::file_type::regular);
    }

    find_options &find_options::is_dir() {
        return type(std::filesystem::file_type::directory);
    }

    find_options &find_options::is_symlink() {
        _conditions.push_back({[](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.symlink_status(ec).type() == std::filesystem::file_type::symlink;
        }});
        return *this;
    }

    find_options &find_options::is_block() {
        return type(std::filesystem::file_type::block);
    }

    find_options &find_options::is_char() {
        return type(std::filesystem::file_type::character);
    }

    find_options &find_options::is_fifo() {
        return type(std::filesystem::file_type::fifo);
    }

    find_options &find_options::is_socket() {
        return type(std::filesystem::file_type::socket);
    }

    find_options &find_options::size_eq(std::uintmax_t size) {
        _conditions.push_back({[size](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            if (!e.is_regular_file(ec)) {
                return false;
            }
            return e.file_size(ec) == size;
        }});
        return *this;
    }

    find_options &find_options::size_gt(std::uintmax_t size) {
        _conditions.push_back({[size](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            if (!e.is_regular_file(ec)) {
                return false;
            }
            return e.file_size(ec) > size;
        }});
        return *this;
    }

    find_options &find_options::size_lt(std::uintmax_t size) {
        _conditions.push_back({[size](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            if (!e.is_regular_file(ec)) {
                return false;
            }
            return e.file_size(ec) < size;
        }});
        return *this;
    }

    find_options &find_options::size_range(std::uintmax_t min_size, std::uintmax_t max_size) {
        _conditions.push_back({[min_size, max_size](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            if (!e.is_regular_file(ec)) {
                return false;
            }
            auto sz = e.file_size(ec);
            return sz >= min_size && sz <= max_size;
        }});
        return *this;
    }

    find_options &find_options::empty() {
        _conditions.push_back({[](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            if (e.is_regular_file(ec)) {
                return e.file_size(ec) == 0;
            }
            if (e.is_directory(ec)) {
                return std::filesystem::is_empty(e.path(), ec);
            }
            return false;
        }});
        return *this;
    }

    find_options &find_options::mtime_newer(file_time_type time) {
        _conditions.push_back({[time](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.last_write_time(ec) > time;
        }});
        return *this;
    }

    find_options &find_options::mtime_older(file_time_type time) {
        _conditions.push_back({[time](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.last_write_time(ec) < time;
        }});
        return *this;
    }

    find_options &find_options::mtime_range(file_time_type start, file_time_type end) {
        _conditions.push_back({[start, end](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            auto t = e.last_write_time(ec);
            return t >= start && t <= end;
        }});
        return *this;
    }

    find_options &find_options::mmin(int minutes) {
        auto threshold = file_time_type::clock::now() - std::chrono::minutes(minutes);
        _conditions.push_back({[threshold](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.last_write_time(ec) >= threshold;
        }});
        return *this;
    }

    find_options &find_options::mtime(int days) {
        auto threshold = file_time_type::clock::now() - std::chrono::hours(days * 24);
        _conditions.push_back({[threshold](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.last_write_time(ec) >= threshold;
        }});
        return *this;
    }

    find_options &find_options::newer_than(const path &p) {
        std::error_code ec;
        auto ref_time = std::filesystem::last_write_time(p, ec);
        if (ec) {
            ref_time = file_time_type::min();
        }
        _conditions.push_back({[ref_time](const std::filesystem::directory_entry &e) {
            std::error_code ec2;
            return e.last_write_time(ec2) > ref_time;
        }});
        return *this;
    }

    find_options &find_options::older_than(const path &p) {
        std::error_code ec;
        auto ref_time = std::filesystem::last_write_time(p, ec);
        if (ec) {
            ref_time = file_time_type::max();
        }
        _conditions.push_back({[ref_time](const std::filesystem::directory_entry &e) {
            std::error_code ec2;
            return e.last_write_time(ec2) < ref_time;
        }});
        return *this;
    }

    find_options &find_options::readable() {
        _conditions.push_back({[](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            auto perms = e.status(ec).permissions();
            return (perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none;
        }});
        return *this;
    }

    find_options &find_options::writable() {
        _conditions.push_back({[](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            auto perms = e.status(ec).permissions();
            return (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
        }});
        return *this;
    }

    find_options &find_options::executable() {
        _conditions.push_back({[](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            auto perms = e.status(ec).permissions();
            return (perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none;
        }});
        return *this;
    }

    find_options &find_options::max_depth(int depth) {
        _max_depth = depth;
        return *this;
    }

    find_options &find_options::min_depth(int depth) {
        _min_depth = depth;
        return *this;
    }

    find_options &find_options::depth_range(int min_d, int max_d) {
        _min_depth = min_d;
        _max_depth = max_d;
        return *this;
    }

    find_options &find_options::links_eq(std::uintmax_t count) {
        _conditions.push_back({[count](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.hard_link_count(ec) == count;
        }});
        return *this;
    }

    find_options &find_options::links_gt(std::uintmax_t count) {
        _conditions.push_back({[count](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.hard_link_count(ec) > count;
        }});
        return *this;
    }

    find_options &find_options::links_lt(std::uintmax_t count) {
        _conditions.push_back({[count](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            return e.hard_link_count(ec) < count;
        }});
        return *this;
    }

    find_options &find_options::broken_symlink() {
        _conditions.push_back({[](const std::filesystem::directory_entry &e) {
            std::error_code ec;
            if (e.symlink_status(ec).type() != std::filesystem::file_type::symlink) {
                return false;
            }
            return !std::filesystem::exists(e.path(), ec);
        }});
        return *this;
    }

    find_options &find_options::xdev() {
        _xdev = true;
        return *this;
    }

    find_options &find_options::predicate(std::function<bool(const std::filesystem::directory_entry &)> pred) {
        _conditions.push_back({std::move(pred)});
        return *this;
    }

    bool find_options::matches(const std::filesystem::directory_entry &entry, int current_depth) const {
        if (current_depth < _min_depth) {
            return false;
        }

        for (const auto &cond : _conditions) {
            if (!cond.predicate(entry)) {
                return false;
            }
        }
        return true;
    }

    // Search implementation
    std::vector<path> find(const path &search_path, const find_options &options) {
        std::error_code ec;
        auto result = find(search_path, options, ec);
        if (ec) {
            throw std::filesystem::filesystem_error("find failed", search_path, ec);
        }
        return result;
    }

    std::vector<path> find(const path &search_path, const find_options &options, std::error_code &ec) {
        std::vector<path> results;
        ec.clear();

        if (!std::filesystem::exists(search_path, ec) || ec) {
            return results;
        }

        struct search_state {
            std::filesystem::directory_iterator iter;
            std::filesystem::directory_iterator end;
            int depth;
        };

        std::stack<search_state> stack;

        std::error_code iter_ec;
        auto root_iter = std::filesystem::directory_iterator(search_path, iter_ec);
        if (iter_ec) {
            ec = iter_ec;
            return results;
        }

        stack.push({root_iter, std::filesystem::directory_iterator{}, 1});

        while (!stack.empty()) {
            auto &current = stack.top();

            if (current.iter == current.end) {
                stack.pop();
                continue;
            }

            auto entry = *current.iter;
            ++current.iter;

            int depth = current.depth;

            if (options.matches(entry, depth)) {
                results.push_back(entry.path());
            }

            std::error_code dir_ec;
            if (entry.is_directory(dir_ec) && !dir_ec) {
                bool should_recurse = true;

                if (options.get_max_depth() >= 0 && depth >= options.get_max_depth()) {
                    should_recurse = false;
                }

                if (entry.symlink_status(dir_ec).type() == std::filesystem::file_type::symlink) {
                    should_recurse = false;
                }

                if (should_recurse) {
                    auto sub_iter = std::filesystem::directory_iterator(entry.path(), dir_ec);
                    if (!dir_ec) {
                        stack.push({sub_iter, std::filesystem::directory_iterator{}, depth + 1});
                    }
                }
            }
        }

        return results;
    }

    // Find iterator implementation
    class find_iterator::impl {
      public:
        impl(const path &search_path, const find_options &options) : _options(options) {

            std::error_code ec;
            if (!std::filesystem::exists(search_path, ec) || ec) {
                return;
            }

            auto root_iter = std::filesystem::directory_iterator(search_path, ec);
            if (ec) {
                return;
            }

            _stack.push({root_iter, std::filesystem::directory_iterator{}, 1});
            advance_to_next_match();
        }

        bool at_end() const { return _stack.empty(); }

        const path &current() const { return _current_path; }

        void advance() {
            if (!_stack.empty()) {
                advance_to_next_match();
            }
        }

      private:
        struct search_state {
            std::filesystem::directory_iterator iter;
            std::filesystem::directory_iterator end;
            int depth;
        };

        void advance_to_next_match() {
            while (!_stack.empty()) {
                auto &current = _stack.top();

                if (current.iter == current.end) {
                    _stack.pop();
                    continue;
                }

                auto entry = *current.iter;
                ++current.iter;

                int depth = current.depth;

                std::error_code ec;
                if (entry.is_directory(ec) && !ec) {
                    bool should_recurse = true;

                    if (_options.get_max_depth() >= 0 && depth >= _options.get_max_depth()) {
                        should_recurse = false;
                    }

                    if (entry.symlink_status(ec).type() == std::filesystem::file_type::symlink) {
                        should_recurse = false;
                    }

                    if (should_recurse) {
                        auto sub_iter = std::filesystem::directory_iterator(entry.path(), ec);
                        if (!ec) {
                            _stack.push({sub_iter, std::filesystem::directory_iterator{}, depth + 1});
                        }
                    }
                }

                if (_options.matches(entry, depth)) {
                    _current_path = entry.path();
                    return;
                }
            }
        }

        find_options _options;
        std::stack<search_state> _stack;
        path _current_path;
    };

    find_iterator::find_iterator(const path &search_path, const find_options &options)
        : _impl(std::make_shared<impl>(search_path, options)) {
        if (_impl->at_end()) {
            _impl.reset();
        }
    }

    find_iterator::reference find_iterator::operator*() const {
        return _impl->current();
    }

    find_iterator::pointer find_iterator::operator->() const {
        return &_impl->current();
    }

    find_iterator &find_iterator::operator++() {
        if (_impl) {
            _impl->advance();
            if (_impl->at_end()) {
                _impl.reset();
            }
        }
        return *this;
    }

    find_iterator find_iterator::operator++(int) {
        find_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool find_iterator::operator==(const find_iterator &other) const {
        return _impl == other._impl;
    }

    bool find_iterator::operator!=(const find_iterator &other) const {
        return !(*this == other);
    }

    find_range::find_range(const path &search_path, const find_options &options)
        : _search_path(search_path), _options(options) {}

    find_iterator find_range::begin() const {
        return find_iterator(_search_path, _options);
    }

    find_iterator find_range::end() const {
        return find_iterator();
    }

    std::optional<path> find_first(const path &search_path, const find_options &options) {
        auto range = find_lazy(search_path, options);
        auto it = range.begin();
        if (it != range.end()) {
            return *it;
        }
        return std::nullopt;
    }

    std::size_t find_count(const path &search_path, const find_options &options) {
        std::size_t count = 0;
        for (const auto &_ : find_lazy(search_path, options)) {
            (void)_;
            ++count;
        }
        return count;
    }

    std::uintmax_t total_size(const std::vector<path> &paths) {
        std::uintmax_t total = 0;
        std::error_code ec;
        for (const auto &p : paths) {
            if (std::filesystem::is_regular_file(p, ec)) {
                total += std::filesystem::file_size(p, ec);
            }
        }
        return total;
    }

} // namespace frappe
