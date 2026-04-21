#ifndef FRAPPE_FORMATTERS_HPP
#define FRAPPE_FORMATTERS_HPP

#include "frappe/entry.hpp"
#include "frappe/filesystem.hpp"
#include "frappe/format_utils.hpp"

#include <format>
#include <string>

// ============================================================================
// std::formatter specializations for frappe types
// ============================================================================

// file_type formatter
template <> struct std::formatter<std::filesystem::file_type> {
    bool use_char = false;

    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == 'c') {
            use_char = true;
            ++it;
        }
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for file_type");
        }
        return it;
    }

    auto format(std::filesystem::file_type ft, std::format_context &ctx) const {
        if (use_char) {
            return std::format_to(ctx.out(), "{}", frappe::format_file_type_char(ft));
        }
        return std::format_to(ctx.out(), "{}", frappe::format_file_type_name(ft));
    }
};

// perms formatter
template <> struct std::formatter<std::filesystem::perms> {
    char mode = 's'; // 's' = string, 'o' = octal

    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 's' || *it == 'o')) {
            mode = *it;
            ++it;
        }
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for perms");
        }
        return it;
    }

    auto format(std::filesystem::perms p, std::format_context &ctx) const {
        if (mode == 'o') {
            return std::format_to(ctx.out(), "{}", frappe::format_permissions_octal(p));
        }
        return std::format_to(ctx.out(), "{}", frappe::format_permissions(p));
    }
};

// filesystem_type formatter
template <> struct std::formatter<frappe::filesystem_type> {
    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for filesystem_type");
        }
        return it;
    }

    auto format(frappe::filesystem_type ft, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "{}", frappe::filesystem_type_name(ft));
    }
};

// file_id formatter
template <> struct std::formatter<frappe::file_id> {
    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for file_id");
        }
        return it;
    }

    auto format(const frappe::file_id &id, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "{}:{}", id.device, id.inode);
    }
};

// link_type formatter
template <> struct std::formatter<frappe::link_type> {
    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for link_type");
        }
        return it;
    }

    auto format(frappe::link_type lt, std::format_context &ctx) const {
        switch (lt) {
        case frappe::link_type::none:
            return std::format_to(ctx.out(), "none");
        case frappe::link_type::symlink:
            return std::format_to(ctx.out(), "symlink");
        case frappe::link_type::hardlink:
            return std::format_to(ctx.out(), "hardlink");
        default:
            return std::format_to(ctx.out(), "unknown");
        }
    }
};

// link_info formatter
template <> struct std::formatter<frappe::link_info> {
    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for link_info");
        }
        return it;
    }

    auto format(const frappe::link_info &info, std::format_context &ctx) const {
        if (info.type == frappe::link_type::symlink) {
            auto out = std::format_to(ctx.out(), "symlink -> {}", info.target.string());
            if (info.is_broken) {
                out = std::format_to(out, " [broken]");
            }
            if (info.is_circular) {
                out = std::format_to(out, " [circular]");
            }
            return out;
        } else if (info.type == frappe::link_type::hardlink) {
            return std::format_to(ctx.out(), "hardlink (count={})", info.link_count);
        }
        return std::format_to(ctx.out(), "regular");
    }
};

// owner_info formatter
template <> struct std::formatter<frappe::owner_info> {
    char mode = 'b'; // 'b' = both, 'u' = user only, 'g' = group only

    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 'u' || *it == 'g' || *it == 'b')) {
            mode = *it;
            ++it;
        }
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for owner_info");
        }
        return it;
    }

    auto format(const frappe::owner_info &info, std::format_context &ctx) const {
        switch (mode) {
        case 'u':
            return std::format_to(ctx.out(), "{}", info.user_name);
        case 'g':
            return std::format_to(ctx.out(), "{}", info.group_name);
        default:
            return std::format_to(ctx.out(), "{}:{}", info.user_name, info.group_name);
        }
    }
};

// space_info formatter
template <> struct std::formatter<std::filesystem::space_info> {
    bool human = false;

    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == 'h') {
            human = true;
            ++it;
        }
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format for space_info");
        }
        return it;
    }

    auto format(const std::filesystem::space_info &info, std::format_context &ctx) const {
        if (human) {
            return std::format_to(ctx.out(), "{}/{} ({})", frappe::format_size_human(info.available),
                                  frappe::format_size_human(info.capacity), frappe::format_size_human(info.free));
        }
        return std::format_to(ctx.out(), "{}/{} ({} free)", info.available, info.capacity, info.free);
    }
};

// file_entry formatter
// Format spec: {:[fields][:modifiers]}
// Fields: n=name, p=path, s=size, t=type, P=perms, o=owner, g=group,
//         m=mtime, a=atime, c=ctime, b=birth, i=inode, l=links, L=target
// Modifiers: h=human size, o=octal perms, r=relative path
template <> struct std::formatter<frappe::file_entry> {
    std::string fields;
    bool human_size = false;
    bool octal_perms = false;
    bool relative_path = false;
    std::string time_format = "%Y-%m-%d %H:%M:%S";

    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin();

        // Parse fields
        while (it != ctx.end() && *it != ':' && *it != '}') {
            fields += *it;
            ++it;
        }

        // Parse modifiers after ':'
        if (it != ctx.end() && *it == ':') {
            ++it;
            while (it != ctx.end() && *it != '}') {
                if (*it == 'h') {
                    human_size = true;
                } else if (*it == 'o') {
                    octal_perms = true;
                } else if (*it == 'r') {
                    relative_path = true;
                } else if (*it == '%') {
                    // Time format starts with %
                    time_format.clear();
                    while (it != ctx.end() && *it != '}') {
                        time_format += *it;
                        ++it;
                    }
                    break;
                }
                ++it;
            }
        }

        // Default to name if no fields specified
        if (fields.empty()) {
            fields = "n";
        }

        return it;
    }

    auto format(const frappe::file_entry &e, std::format_context &ctx) const {
        auto out = ctx.out();
        bool first = true;

        for (char field : fields) {
            if (!first) {
                out = std::format_to(out, " ");
            }
            first = false;

            switch (field) {
            case 'n': // name
                out = std::format_to(out, "{}", e.name);
                break;
            case 'p': // path
                if (relative_path) {
                    out = std::format_to(out, "{}", e.file_path.filename().string());
                } else {
                    out = std::format_to(out, "{}", e.file_path.string());
                }
                break;
            case 's': // size
                if (human_size) {
                    out = std::format_to(out, "{}", frappe::format_size_human(e.size));
                } else {
                    out = std::format_to(out, "{}", e.size);
                }
                break;
            case 't': // type
                out = std::format_to(out, "{}", frappe::format_file_type_char(e.type));
                break;
            case 'P': // permissions
                if (octal_perms) {
                    out = std::format_to(out, "{}", frappe::format_permissions_octal(e.permissions));
                } else {
                    out = std::format_to(out, "{}{}", frappe::format_file_type_char(e.type),
                                         frappe::format_permissions(e.permissions));
                }
                break;
            case 'o': // owner
                out = std::format_to(out, "{}", e.owner);
                break;
            case 'g': // group
                out = std::format_to(out, "{}", e.group);
                break;
            case 'm': // mtime
                out = std::format_to(out, "{}", frappe::format_time(e.mtime, time_format));
                break;
            case 'a': // atime
                out = std::format_to(out, "{}", frappe::format_time(e.atime, time_format));
                break;
            case 'c': // ctime
                out = std::format_to(out, "{}", frappe::format_time(e.ctime, time_format));
                break;
            case 'b': // birth time
                out = std::format_to(out, "{}", frappe::format_time(e.birth_time, time_format));
                break;
            case 'i': // inode
                out = std::format_to(out, "{}", e.inode);
                break;
            case 'l': // link count
                out = std::format_to(out, "{}", e.hard_link_count);
                break;
            case 'L': // symlink target
                if (e.is_symlink) {
                    out = std::format_to(out, "-> {}", e.symlink_target.string());
                }
                break;
            case '+': // ACL indicator
                out = std::format_to(out, "{}", e.has_acl ? '+' : ' ');
                break;
            case '@': // xattr indicator
                out = std::format_to(out, "{}", e.has_xattr ? '@' : ' ');
                break;
            default:
                // Unknown field, skip
                first = true; // Don't add space for next
                break;
            }
        }

        return out;
    }
};

// ============================================================================
// Predefined format strings for common use cases
// ============================================================================

namespace frappe::fmt {
// ls-style formats
inline constexpr std::string_view ls_short = "{:n}";
inline constexpr std::string_view ls_long = "{:Plogsmn}";
inline constexpr std::string_view ls_human = "{:Plogsmn:h}";
inline constexpr std::string_view ls_inode = "{:iPlogsmn}";
inline constexpr std::string_view ls_all = "{:iPlogsmn+@L}";

// Custom formats
inline constexpr std::string_view name_only = "{:n}";
inline constexpr std::string_view path_only = "{:p}";
inline constexpr std::string_view size_name = "{:sn:h}";
inline constexpr std::string_view time_name = "{:mn}";
} // namespace frappe::fmt

#endif // FRAPPE_FORMATTERS_HPP
