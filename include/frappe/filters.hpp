#ifndef FRAPPE_FILTERS_HPP
#define FRAPPE_FILTERS_HPP

#include "frappe/entry.hpp"
#include "frappe/projections.hpp"
#include "frappe/regex.hpp"

#include <cctype>
#include <chrono>
#include <fstream>
#include <functional>
#include <ranges>
#include <span>

namespace frappe {

    // ============================================================================
    // Generic filter adapter
    // ============================================================================

    template <typename Pred> struct filter_adapter {
        Pred predicate;

        template <std::ranges::input_range R> auto operator()(R &&r) const {
            return std::forward<R>(r) | std::views::filter(predicate);
        }

        template <std::ranges::input_range R> friend auto operator|(R &&r, const filter_adapter &f) {
            return f(std::forward<R>(r));
        }
    };

    template <typename Pred> [[nodiscard]] inline filter_adapter<Pred> filter(Pred pred) {
        return filter_adapter<Pred>{std::move(pred)};
    }

    // ============================================================================
    // Boolean combination operators for filters
    // ============================================================================

    // AND combinator
    template <typename P1, typename P2> struct and_predicate {
        P1 pred1;
        P2 pred2;

        template <typename T> bool operator()(const T &e) const { return pred1(e) && pred2(e); }
    };

    template <typename P1, typename P2>
    [[nodiscard]] inline auto operator&&(filter_adapter<P1> f1, filter_adapter<P2> f2) {
        return filter(and_predicate<P1, P2>{std::move(f1.predicate), std::move(f2.predicate)});
    }

    // OR combinator
    template <typename P1, typename P2> struct or_predicate {
        P1 pred1;
        P2 pred2;

        template <typename T> bool operator()(const T &e) const { return pred1(e) || pred2(e); }
    };

    template <typename P1, typename P2>
    [[nodiscard]] inline auto operator||(filter_adapter<P1> f1, filter_adapter<P2> f2) {
        return filter(or_predicate<P1, P2>{std::move(f1.predicate), std::move(f2.predicate)});
    }

    // NOT combinator
    template <typename P> struct not_predicate {
        P pred;

        template <typename T> bool operator()(const T &e) const { return !pred(e); }
    };

    template <typename P> [[nodiscard]] inline auto operator!(filter_adapter<P> f) {
        return filter(not_predicate<P>{std::move(f.predicate)});
    }

    // Functional combinators (alternative syntax)
    template <typename P1, typename P2> [[nodiscard]] inline auto and_(filter_adapter<P1> f1, filter_adapter<P2> f2) {
        return f1 && f2;
    }

    template <typename P1, typename P2> [[nodiscard]] inline auto or_(filter_adapter<P1> f1, filter_adapter<P2> f2) {
        return f1 || f2;
    }

    template <typename P> [[nodiscard]] inline auto not_(filter_adapter<P> f) {
        return !f;
    }

    // ============================================================================
    // Type filters
    // ============================================================================

    inline const auto files_only = filter([](const file_entry &e) { return e.type == file_type::regular; });
    inline const auto dirs_only = filter([](const file_entry &e) { return e.type == file_type::directory; });
    inline const auto symlinks_only = filter([](const file_entry &e) { return e.is_symlink; });
    inline const auto hidden_only = filter([](const file_entry &e) { return e.is_hidden; });
    inline const auto no_hidden = filter([](const file_entry &e) { return !e.is_hidden; });
    inline const auto broken_links = filter([](const file_entry &e) { return e.is_broken_link; });

    [[nodiscard]] inline auto type_is(file_type ft) {
        return filter([ft](const file_entry &e) { return e.type == ft; });
    }

    // ============================================================================
    // Size filters
    // ============================================================================

    [[nodiscard]] inline auto size_gt(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.size > n; });
    }

    [[nodiscard]] inline auto size_lt(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.size < n; });
    }

    [[nodiscard]] inline auto size_eq(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.size == n; });
    }

    [[nodiscard]] inline auto size_ge(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.size >= n; });
    }

    [[nodiscard]] inline auto size_le(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.size <= n; });
    }

    [[nodiscard]] inline auto size_range(std::uintmax_t min_size, std::uintmax_t max_size) {
        return filter([min_size, max_size](const file_entry &e) { return e.size >= min_size && e.size <= max_size; });
    }

    inline const auto empty_files =
        filter([](const file_entry &e) { return e.type == file_type::regular && e.size == 0; });

    inline const auto non_empty = filter([](const file_entry &e) { return e.size > 0; });

    // ============================================================================
    // Time filters
    // ============================================================================

    [[nodiscard]] inline auto newer_than(file_time_type t) {
        return filter([t](const file_entry &e) { return e.mtime > t; });
    }

    [[nodiscard]] inline auto older_than(file_time_type t) {
        return filter([t](const file_entry &e) { return e.mtime < t; });
    }

    [[nodiscard]] inline auto mtime_range(file_time_type start, file_time_type end) {
        return filter([start, end](const file_entry &e) { return e.mtime >= start && e.mtime <= end; });
    }

    template <typename Duration> [[nodiscard]] inline auto modified_within(Duration d) {
        auto threshold = file_time_type::clock::now() - d;
        return filter([threshold](const file_entry &e) { return e.mtime > threshold; });
    }

    template <typename Duration> [[nodiscard]] inline auto accessed_within(Duration d) {
        auto threshold = file_time_type::clock::now() - d;
        return filter([threshold](const file_entry &e) { return e.atime > threshold; });
    }

    template <typename Duration> [[nodiscard]] inline auto created_within(Duration d) {
        auto threshold = file_time_type::clock::now() - d;
        return filter([threshold](const file_entry &e) { return e.birth_time > threshold; });
    }

    // Convenience functions for common time ranges
    [[nodiscard]] inline auto modified_today() {
        return modified_within(std::chrono::hours(24));
    }

    [[nodiscard]] inline auto modified_this_week() {
        return modified_within(std::chrono::hours(24 * 7));
    }

    [[nodiscard]] inline auto modified_this_month() {
        return modified_within(std::chrono::hours(24 * 30));
    }

    // ============================================================================
    // Permission filters
    // ============================================================================

    inline const auto readable =
        filter([](const file_entry &e) { return (e.permissions & perms::owner_read) != perms::none; });

    inline const auto writable =
        filter([](const file_entry &e) { return (e.permissions & perms::owner_write) != perms::none; });

    inline const auto executable =
        filter([](const file_entry &e) { return (e.permissions & perms::owner_exec) != perms::none; });

    [[nodiscard]] inline auto has_permission(perms p) {
        return filter([p](const file_entry &e) { return (e.permissions & p) != perms::none; });
    }

    // ============================================================================
    // Name/Extension filters
    // ============================================================================

    [[nodiscard]] inline auto extension_is(std::string_view ext) {
        std::string ext_str(ext);
        if (!ext_str.empty() && ext_str[0] != '.') {
            ext_str = "." + ext_str;
        }
        return filter([ext_str = std::move(ext_str)](const file_entry &e) { return e.extension == ext_str; });
    }

    [[nodiscard]] inline auto extension_in(std::initializer_list<std::string_view> exts) {
        std::vector<std::string> ext_list;
        for (auto ext : exts) {
            std::string s(ext);
            if (!s.empty() && s[0] != '.') {
                s = "." + s;
            }
            ext_list.push_back(std::move(s));
        }
        return filter([ext_list = std::move(ext_list)](const file_entry &e) {
            return std::find(ext_list.begin(), ext_list.end(), e.extension) != ext_list.end();
        });
    }

    [[nodiscard]] inline auto name_contains(std::string_view substr) {
        std::string s(substr);
        return filter([s = std::move(s)](const file_entry &e) { return e.name.find(s) != std::string::npos; });
    }

    [[nodiscard]] inline auto path_contains(std::string_view substr) {
        std::string s(substr);
        return filter(
            [s = std::move(s)](const file_entry &e) { return e.file_path.string().find(s) != std::string::npos; });
    }

    [[nodiscard]] inline auto name_starts_with(std::string_view prefix) {
        std::string s(prefix);
        return filter([s = std::move(s)](const file_entry &e) { return e.name.starts_with(s); });
    }

    [[nodiscard]] inline auto name_ends_with(std::string_view suffix) {
        std::string s(suffix);
        return filter([s = std::move(s)](const file_entry &e) { return e.name.ends_with(s); });
    }

    // Regex filter for file names
    [[nodiscard]] inline auto name_regex(std::string_view pattern) {
        auto re = std::make_shared<compiled_regex>(pattern);
        return filter([re](const file_entry &e) { return re->match(e.name); });
    }

    // Regex filter for full path
    [[nodiscard]] inline auto path_regex(std::string_view pattern) {
        auto re = std::make_shared<compiled_regex>(pattern);
        return filter([re](const file_entry &e) { return re->match(e.file_path.string()); });
    }

    // ============================================================================
    // Ownership filters
    // ============================================================================

    [[nodiscard]] inline auto owned_by(std::string_view user) {
        std::string u(user);
        return filter([u = std::move(u)](const file_entry &e) { return e.owner == u; });
    }

    [[nodiscard]] inline auto group_is(std::string_view grp) {
        std::string g(grp);
        return filter([g = std::move(g)](const file_entry &e) { return e.group == g; });
    }

    [[nodiscard]] inline auto uid_is(std::uint32_t id) {
        return filter([id](const file_entry &e) { return e.uid == id; });
    }

    [[nodiscard]] inline auto gid_is(std::uint32_t id) {
        return filter([id](const file_entry &e) { return e.gid == id; });
    }

    // ============================================================================
    // Link filters
    // ============================================================================

    [[nodiscard]] inline auto links_eq(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.hard_link_count == n; });
    }

    [[nodiscard]] inline auto links_gt(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.hard_link_count > n; });
    }

    [[nodiscard]] inline auto links_lt(std::uintmax_t n) {
        return filter([n](const file_entry &e) { return e.hard_link_count < n; });
    }

    inline const auto has_multiple_links = filter([](const file_entry &e) { return e.hard_link_count > 1; });

    // ============================================================================
    // Extended attribute filters
    // ============================================================================

    inline const auto with_acl = filter([](const file_entry &e) { return e.has_acl; });
    inline const auto with_xattr = filter([](const file_entry &e) { return e.has_xattr; });

    // ============================================================================
    // Filesystem type filters
    // ============================================================================

    [[nodiscard]] inline auto on_filesystem(filesystem_type ft) {
        return filter([ft](const file_entry &e) { return e.fs_type == ft; });
    }

    // ============================================================================
    // Depth filters
    // ============================================================================

    [[nodiscard]] inline auto depth_eq(int n) {
        return filter([n](const file_entry &e) {
            auto depth = static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end()));
            return depth == n;
        });
    }

    [[nodiscard]] inline auto depth_lt(int n) {
        return filter([n](const file_entry &e) {
            auto depth = static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end()));
            return depth < n;
        });
    }

    [[nodiscard]] inline auto depth_gt(int n) {
        return filter([n](const file_entry &e) {
            auto depth = static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end()));
            return depth > n;
        });
    }

    [[nodiscard]] inline auto depth_le(int n) {
        return filter([n](const file_entry &e) {
            auto depth = static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end()));
            return depth <= n;
        });
    }

    [[nodiscard]] inline auto depth_ge(int n) {
        return filter([n](const file_entry &e) {
            auto depth = static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end()));
            return depth >= n;
        });
    }

    [[nodiscard]] inline auto depth_range(int min_depth, int max_depth) {
        return filter([min_depth, max_depth](const file_entry &e) {
            auto depth = static_cast<int>(std::distance(e.file_path.begin(), e.file_path.end()));
            return depth >= min_depth && depth <= max_depth;
        });
    }

    // ============================================================================
    // Name matching filters (exact, case-insensitive)
    // ============================================================================

    [[nodiscard]] inline auto name_eq(std::string_view str) {
        std::string s(str);
        return filter([s = std::move(s)](const file_entry &e) { return e.name == s; });
    }

    [[nodiscard]] inline auto name_icase(std::string_view str) {
        std::string s(str);
        return filter([s = std::move(s)](const file_entry &e) {
            if (e.name.size() != s.size()) {
                return false;
            }
            return std::equal(e.name.begin(), e.name.end(), s.begin(), s.end(),
                              [](char a, char b) { return std::tolower(a) == std::tolower(b); });
        });
    }

    [[nodiscard]] inline auto stem_eq(std::string_view str) {
        std::string s(str);
        return filter([s = std::move(s)](const file_entry &e) { return e.stem == s; });
    }

    [[nodiscard]] inline auto stem_icase(std::string_view str) {
        std::string s(str);
        return filter([s = std::move(s)](const file_entry &e) {
            if (e.stem.size() != s.size()) {
                return false;
            }
            return std::equal(e.stem.begin(), e.stem.end(), s.begin(), s.end(),
                              [](char a, char b) { return std::tolower(a) == std::tolower(b); });
        });
    }

    // ============================================================================
    // Parent path filters
    // ============================================================================

    [[nodiscard]] inline auto parent_is(const path &p) {
        return filter([p](const file_entry &e) { return e.file_path.parent_path() == p; });
    }

    [[nodiscard]] inline auto under_path(const path &p) {
        std::string prefix = p.string();
        return filter(
            [prefix = std::move(prefix)](const file_entry &e) { return e.file_path.string().starts_with(prefix); });
    }

    // ============================================================================
    // File comparison filters
    // ============================================================================

    [[nodiscard]] inline auto is_newer_than(const path &p) {
        std::error_code ec;
        auto ref_time = std::filesystem::last_write_time(p, ec);
        return filter([ref_time](const file_entry &e) { return e.mtime > ref_time; });
    }

    [[nodiscard]] inline auto is_older_than(const path &p) {
        std::error_code ec;
        auto ref_time = std::filesystem::last_write_time(p, ec);
        return filter([ref_time](const file_entry &e) { return e.mtime < ref_time; });
    }

    [[nodiscard]] inline auto same_inode_as(const path &p) {
        auto id_result = get_file_id(p);
        std::uint64_t ref_inode = id_result ? id_result->inode : 0;
        bool valid = id_result.has_value();
        return filter([ref_inode, valid](const file_entry &e) { return valid && e.inode == ref_inode; });
    }

    [[nodiscard]] inline auto same_size_as(const path &p) {
        std::error_code ec;
        auto ref_size = std::filesystem::file_size(p, ec);
        return filter([ref_size](const file_entry &e) { return e.size == ref_size; });
    }

    // ============================================================================
    // Content filters (magic number / file header)
    // ============================================================================

    [[nodiscard]] inline auto has_magic(std::span<const std::byte> magic) {
        std::vector<std::byte> magic_bytes(magic.begin(), magic.end());
        return filter([magic_bytes = std::move(magic_bytes)](const file_entry &e) {
            if (e.type != file_type::regular || e.size < magic_bytes.size()) {
                return false;
            }
            std::ifstream file(e.file_path, std::ios::binary);
            if (!file) {
                return false;
            }
            std::vector<char> buffer(magic_bytes.size());
            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            if (file.gcount() != static_cast<std::streamsize>(magic_bytes.size())) {
                return false;
            }
            return std::equal(buffer.begin(), buffer.end(), reinterpret_cast<const char *>(magic_bytes.data()));
        });
    }

    [[nodiscard]] inline auto has_magic(std::initializer_list<unsigned char> magic) {
        std::vector<std::byte> magic_bytes;
        magic_bytes.reserve(magic.size());
        for (auto b : magic) {
            magic_bytes.push_back(static_cast<std::byte>(b));
        }
        return has_magic(std::span<const std::byte>(magic_bytes));
    }

    // Common magic numbers
    inline const auto is_pdf = has_magic({0x25, 0x50, 0x44, 0x46}); // %PDF
    inline const auto is_png = has_magic({0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A});
    inline const auto is_jpeg = has_magic({0xFF, 0xD8, 0xFF});
    inline const auto is_gif = has_magic({0x47, 0x49, 0x46, 0x38}); // GIF8
    inline const auto is_zip = has_magic({0x50, 0x4B, 0x03, 0x04}); // PK
    inline const auto is_gzip = has_magic({0x1F, 0x8B});
    inline const auto is_elf = has_magic({0x7F, 0x45, 0x4C, 0x46}); // ELF
    inline const auto is_pe = has_magic({0x4D, 0x5A});              // MZ (Windows PE)

} // namespace frappe

#endif // FRAPPE_FILTERS_HPP
