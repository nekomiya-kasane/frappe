#pragma once

#include "entry.hpp"

#include <rpp/rpp.hpp>

namespace frappe {

// ============================================================================
// file_entry 投影定义 - 用于rpp适配器
// ============================================================================

namespace proj {

inline constexpr auto path = [](const file_entry &e) -> const frappe::path & { return e.file_path; };

inline constexpr auto name = [](const file_entry &e) -> const std::string & { return e.name; };

inline constexpr auto stem = [](const file_entry &e) -> const std::string & { return e.stem; };

inline constexpr auto extension = [](const file_entry &e) -> const std::string & { return e.extension; };

inline constexpr auto type = [](const file_entry &e) { return e.type; };

inline constexpr auto size = [](const file_entry &e) { return e.size; };

inline constexpr auto mtime = [](const file_entry &e) { return e.mtime; };

inline constexpr auto atime = [](const file_entry &e) { return e.atime; };

inline constexpr auto ctime = [](const file_entry &e) { return e.ctime; };

inline constexpr auto birth_time = [](const file_entry &e) { return e.birth_time; };

inline constexpr auto permissions = [](const file_entry &e) { return e.permissions; };

inline constexpr auto owner = [](const file_entry &e) -> const std::string & { return e.owner; };

inline constexpr auto group = [](const file_entry &e) -> const std::string & { return e.group; };

inline constexpr auto uid = [](const file_entry &e) { return e.uid; };

inline constexpr auto gid = [](const file_entry &e) { return e.gid; };

inline constexpr auto inode = [](const file_entry &e) { return e.inode; };

inline constexpr auto hard_link_count = [](const file_entry &e) { return e.hard_link_count; };

inline constexpr auto is_symlink = [](const file_entry &e) { return e.is_symlink; };

inline constexpr auto is_hidden = [](const file_entry &e) { return e.is_hidden; };

inline constexpr auto symlink_target = [](const file_entry &e) -> const frappe::path & { return e.symlink_target; };

inline constexpr auto is_broken_link = [](const file_entry &e) { return e.is_broken_link; };

inline constexpr auto mime_type = [](const file_entry &e) -> const std::string & { return e.mime_type; };

inline constexpr auto fs_type = [](const file_entry &e) { return e.fs_type; };

inline constexpr auto depth = [](const file_entry &e) { return std::distance(e.file_path.begin(), e.file_path.end()); };

inline constexpr auto parent = [](const file_entry &e) { return e.file_path.parent_path(); };

} // namespace proj

// ============================================================================
// file_entry 特化过滤器
// ============================================================================

namespace entry {

// Type filters
inline const auto files_only = rpp::filter([](const file_entry &e) { return e.type == file_type::regular; });

inline const auto dirs_only = rpp::filter([](const file_entry &e) { return e.type == file_type::directory; });

inline const auto symlinks_only = rpp::filter([](const file_entry &e) { return e.is_symlink; });

inline const auto hidden_only = rpp::filter([](const file_entry &e) { return e.is_hidden; });

inline const auto visible_only = rpp::filter([](const file_entry &e) { return !e.is_hidden; });

inline const auto broken_links_only = rpp::filter([](const file_entry &e) { return e.is_broken_link; });

// Size filters
[[nodiscard]] inline auto size_gt(std::uintmax_t n) {
    return rpp::filters::gt(proj::size, n);
}

[[nodiscard]] inline auto size_ge(std::uintmax_t n) {
    return rpp::filters::ge(proj::size, n);
}

[[nodiscard]] inline auto size_lt(std::uintmax_t n) {
    return rpp::filters::lt(proj::size, n);
}

[[nodiscard]] inline auto size_le(std::uintmax_t n) {
    return rpp::filters::le(proj::size, n);
}

[[nodiscard]] inline auto size_eq(std::uintmax_t n) {
    return rpp::filters::eq(proj::size, n);
}

[[nodiscard]] inline auto size_between(std::uintmax_t lo, std::uintmax_t hi) {
    return rpp::filters::between(proj::size, lo, hi);
}

// Extension filters
[[nodiscard]] inline auto extension_is(std::string_view ext) {
    std::string e = ext.starts_with('.') ? std::string(ext) : "." + std::string(ext);
    return rpp::filters::eq(proj::extension, std::move(e));
}

[[nodiscard]] inline auto extension_in(std::vector<std::string> exts) {
    for (auto &e : exts) {
        if (!e.starts_with('.')) e = "." + e;
    }
    return rpp::filters::in(proj::extension, std::move(exts));
}

// Name filters
[[nodiscard]] inline auto name_is(std::string_view n) {
    return rpp::filters::eq(proj::name, std::string(n));
}

[[nodiscard]] inline auto name_contains(std::string_view substr) {
    return rpp::filters::contains(proj::name, std::string(substr));
}

[[nodiscard]] inline auto name_starts_with(std::string_view prefix) {
    return rpp::filters::starts_with(proj::name, prefix);
}

[[nodiscard]] inline auto name_ends_with(std::string_view suffix) {
    return rpp::filters::ends_with(proj::name, suffix);
}

[[nodiscard]] inline auto name_matches(std::string_view pattern) {
    return rpp::filters::matches(proj::name, pattern);
}

[[nodiscard]] inline auto name_like(std::string_view pattern) {
    return rpp::filters::like(proj::name, pattern);
}

// Time filters
template <typename TimePoint> [[nodiscard]] inline auto modified_after(TimePoint tp) {
    return rpp::filter([tp](const file_entry &e) { return e.mtime > tp; });
}

template <typename TimePoint> [[nodiscard]] inline auto modified_before(TimePoint tp) {
    return rpp::filter([tp](const file_entry &e) { return e.mtime < tp; });
}

template <typename TimePoint> [[nodiscard]] inline auto accessed_after(TimePoint tp) {
    return rpp::filter([tp](const file_entry &e) { return e.atime > tp; });
}

template <typename TimePoint> [[nodiscard]] inline auto created_after(TimePoint tp) {
    return rpp::filter([tp](const file_entry &e) { return e.birth_time > tp; });
}

// Permission filters
[[nodiscard]] inline auto has_permission(perms p) {
    return rpp::filter([p](const file_entry &e) { return (e.permissions & p) == p; });
}

inline const auto is_readable =
    rpp::filter([](const file_entry &e) { return (e.permissions & perms::owner_read) != perms::none; });

inline const auto is_writable =
    rpp::filter([](const file_entry &e) { return (e.permissions & perms::owner_write) != perms::none; });

inline const auto is_executable =
    rpp::filter([](const file_entry &e) { return (e.permissions & perms::owner_exec) != perms::none; });

// Owner filters
[[nodiscard]] inline auto owned_by(std::string_view owner) {
    return rpp::filters::eq(proj::owner, std::string(owner));
}

[[nodiscard]] inline auto owned_by_uid(std::uint32_t uid) {
    return rpp::filters::eq(proj::uid, uid);
}

[[nodiscard]] inline auto in_group(std::string_view group) {
    return rpp::filters::eq(proj::group, std::string(group));
}

// Inode filter
[[nodiscard]] inline auto same_inode_as(const file_entry &ref) {
    return rpp::filter([inode = ref.inode](const file_entry &e) { return e.inode == inode && inode != 0; });
}

// ============================================================================
// file_entry 特化排序器
// ============================================================================

inline const auto sort_by_name = rpp::sort_by(proj::name);
inline const auto sort_by_size = rpp::sort_by(proj::size);
inline const auto sort_by_mtime = rpp::sort_by(proj::mtime);
inline const auto sort_by_atime = rpp::sort_by(proj::atime);
inline const auto sort_by_ctime = rpp::sort_by(proj::ctime);
inline const auto sort_by_extension = rpp::sort_by(proj::extension);
inline const auto sort_by_path = rpp::sort_by(proj::path);

inline const auto natural_sort_by_name = rpp::natural_sort(proj::name);

// ============================================================================
// file_entry 特化比较器 (用于STL容器)
// ============================================================================

inline const auto compare_by_name = rpp::compare_by(proj::name);
inline const auto compare_by_size = rpp::compare_by(proj::size);
inline const auto compare_by_mtime = rpp::compare_by(proj::mtime);
inline const auto compare_by_path = rpp::compare_by(proj::path);

inline const auto natural_compare_by_name = rpp::natural_compare(proj::name);

// ============================================================================
// file_entry 特化聚合器
// ============================================================================

inline const auto total_size = rpp::sum(proj::size);
inline const auto avg_size = rpp::avg(proj::size);
inline const auto max_size = rpp::max_by(proj::size);
inline const auto min_size = rpp::min_by(proj::size);

inline const auto newest = rpp::max_by(proj::mtime);
inline const auto oldest = rpp::min_by(proj::mtime);

inline const auto group_by_extension = rpp::group_by(proj::extension);
inline const auto group_by_type = rpp::group_by(proj::type);
inline const auto group_by_owner = rpp::group_by(proj::owner);

inline const auto count_by_extension = rpp::count_by(proj::extension);
inline const auto count_by_type = rpp::count_by(proj::type);

// ============================================================================
// 便捷别名
// ============================================================================

using rpp::count;
using rpp::distinct;
using rpp::drop;
using rpp::first;
using rpp::last;
using rpp::skip;
using rpp::take;
using rpp::to_vector;

} // namespace entry

} // namespace frappe
