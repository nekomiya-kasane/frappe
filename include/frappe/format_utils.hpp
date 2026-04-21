#ifndef FRAPPE_FORMAT_UTILS_HPP
#define FRAPPE_FORMAT_UTILS_HPP

#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace frappe {

using file_time_type = std::filesystem::file_time_type;

// ============================================================================
// Size formatting
// ============================================================================

enum class size_unit {
    bytes,
    KB, // 1000
    MB,
    GB,
    TB,
    PB,
    KiB, // 1024
    MiB,
    GiB,
    TiB,
    PiB,
    auto_si, // Auto-select SI units (1000)
    auto_iec // Auto-select IEC units (1024)
};

[[nodiscard]] inline std::string format_size(std::uintmax_t bytes, size_unit unit = size_unit::auto_iec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);

    switch (unit) {
    case size_unit::bytes:
        oss << bytes << " B";
        break;
    case size_unit::KB:
        oss << (bytes / 1000.0) << " KB";
        break;
    case size_unit::MB:
        oss << (bytes / 1000000.0) << " MB";
        break;
    case size_unit::GB:
        oss << (bytes / 1000000000.0) << " GB";
        break;
    case size_unit::TB:
        oss << (bytes / 1000000000000.0) << " TB";
        break;
    case size_unit::PB:
        oss << (bytes / 1000000000000000.0) << " PB";
        break;
    case size_unit::KiB:
        oss << (bytes / 1024.0) << " KiB";
        break;
    case size_unit::MiB:
        oss << (bytes / (1024.0 * 1024)) << " MiB";
        break;
    case size_unit::GiB:
        oss << (bytes / (1024.0 * 1024 * 1024)) << " GiB";
        break;
    case size_unit::TiB:
        oss << (bytes / (1024.0 * 1024 * 1024 * 1024)) << " TiB";
        break;
    case size_unit::PiB:
        oss << (bytes / (1024.0 * 1024 * 1024 * 1024 * 1024)) << " PiB";
        break;
    case size_unit::auto_si: {
        const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
        double size = static_cast<double>(bytes);
        int i = 0;
        while (size >= 1000.0 && i < 5) {
            size /= 1000.0;
            ++i;
        }
        if (i == 0) {
            oss << std::setprecision(0);
        }
        oss << size << " " << units[i];
        break;
    }
    case size_unit::auto_iec:
    default: {
        const char *units[] = {"B", "K", "M", "G", "T", "P"};
        double size = static_cast<double>(bytes);
        int i = 0;
        while (size >= 1024.0 && i < 5) {
            size /= 1024.0;
            ++i;
        }
        if (i == 0) {
            oss << std::setprecision(0);
        }
        oss << size << units[i];
        break;
    }
    }

    return oss.str();
}

// Short human-readable format (like ls -h)
[[nodiscard]] inline std::string format_size_human(std::uintmax_t bytes) {
    return format_size(bytes, size_unit::auto_iec);
}

// SI units (1000-based)
[[nodiscard]] inline std::string format_size_si(std::uintmax_t bytes) {
    return format_size(bytes, size_unit::auto_si);
}

// IEC units (1024-based)
[[nodiscard]] inline std::string format_size_iec(std::uintmax_t bytes) {
    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double size = static_cast<double>(bytes);
    int i = 0;
    while (size >= 1024.0 && i < 5) {
        size /= 1024.0;
        ++i;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(i == 0 ? 0 : 1) << size << " " << units[i];
    return oss.str();
}

// ============================================================================
// Time formatting
// ============================================================================

[[nodiscard]] inline std::string format_time(file_time_type ftime, std::string_view fmt = "%Y-%m-%d %H:%M:%S") {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - file_time_type::clock::now() +
                                                                                  std::chrono::system_clock::now());
    auto time = std::chrono::system_clock::to_time_t(sctp);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, std::string(fmt).c_str());
    return oss.str();
}

// ISO 8601 format
[[nodiscard]] inline std::string format_time_iso(file_time_type ftime) {
    return format_time(ftime, "%Y-%m-%dT%H:%M:%S");
}

// ls-style format (recent files show time, older files show year)
[[nodiscard]] inline std::string format_time_ls(file_time_type ftime) {
    auto now = file_time_type::clock::now();
    auto six_months = std::chrono::hours(24 * 180);

    if (ftime > now - six_months) {
        return format_time(ftime, "%b %d %H:%M");
    } else {
        return format_time(ftime, "%b %d  %Y");
    }
}

// Relative time format
[[nodiscard]] inline std::string format_time_relative(file_time_type ftime) {
    auto now = file_time_type::clock::now();
    auto diff = now - ftime;

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(diff).count();
    auto days = hours / 24;

    if (seconds < 0) {
        return "in the future";
    } else if (seconds < 60) {
        return "just now";
    } else if (minutes < 60) {
        return std::to_string(minutes) + " minute" + (minutes == 1 ? "" : "s") + " ago";
    } else if (hours < 24) {
        return std::to_string(hours) + " hour" + (hours == 1 ? "" : "s") + " ago";
    } else if (days < 7) {
        return std::to_string(days) + " day" + (days == 1 ? "" : "s") + " ago";
    } else if (days < 30) {
        auto weeks = days / 7;
        return std::to_string(weeks) + " week" + (weeks == 1 ? "" : "s") + " ago";
    } else if (days < 365) {
        auto months = days / 30;
        return std::to_string(months) + " month" + (months == 1 ? "" : "s") + " ago";
    } else {
        auto years = days / 365;
        return std::to_string(years) + " year" + (years == 1 ? "" : "s") + " ago";
    }
}

// ============================================================================
// Permission formatting
// ============================================================================

[[nodiscard]] inline std::string format_permissions(std::filesystem::perms p) {
    std::string result;
    result.reserve(9);

    result += (p & std::filesystem::perms::owner_read) != std::filesystem::perms::none ? 'r' : '-';
    result += (p & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? 'w' : '-';
    result += (p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ? 'x' : '-';
    result += (p & std::filesystem::perms::group_read) != std::filesystem::perms::none ? 'r' : '-';
    result += (p & std::filesystem::perms::group_write) != std::filesystem::perms::none ? 'w' : '-';
    result += (p & std::filesystem::perms::group_exec) != std::filesystem::perms::none ? 'x' : '-';
    result += (p & std::filesystem::perms::others_read) != std::filesystem::perms::none ? 'r' : '-';
    result += (p & std::filesystem::perms::others_write) != std::filesystem::perms::none ? 'w' : '-';
    result += (p & std::filesystem::perms::others_exec) != std::filesystem::perms::none ? 'x' : '-';

    // Handle special bits
    if ((p & std::filesystem::perms::set_uid) != std::filesystem::perms::none) {
        result[2] = (result[2] == 'x') ? 's' : 'S';
    }
    if ((p & std::filesystem::perms::set_gid) != std::filesystem::perms::none) {
        result[5] = (result[5] == 'x') ? 's' : 'S';
    }
    if ((p & std::filesystem::perms::sticky_bit) != std::filesystem::perms::none) {
        result[8] = (result[8] == 'x') ? 't' : 'T';
    }

    return result;
}

[[nodiscard]] inline std::string format_permissions_octal(std::filesystem::perms p) {
    unsigned mode = 0;
    if ((p & std::filesystem::perms::owner_read) != std::filesystem::perms::none) {
        mode |= 0400;
    }
    if ((p & std::filesystem::perms::owner_write) != std::filesystem::perms::none) {
        mode |= 0200;
    }
    if ((p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none) {
        mode |= 0100;
    }
    if ((p & std::filesystem::perms::group_read) != std::filesystem::perms::none) {
        mode |= 0040;
    }
    if ((p & std::filesystem::perms::group_write) != std::filesystem::perms::none) {
        mode |= 0020;
    }
    if ((p & std::filesystem::perms::group_exec) != std::filesystem::perms::none) {
        mode |= 0010;
    }
    if ((p & std::filesystem::perms::others_read) != std::filesystem::perms::none) {
        mode |= 0004;
    }
    if ((p & std::filesystem::perms::others_write) != std::filesystem::perms::none) {
        mode |= 0002;
    }
    if ((p & std::filesystem::perms::others_exec) != std::filesystem::perms::none) {
        mode |= 0001;
    }
    if ((p & std::filesystem::perms::set_uid) != std::filesystem::perms::none) {
        mode |= 04000;
    }
    if ((p & std::filesystem::perms::set_gid) != std::filesystem::perms::none) {
        mode |= 02000;
    }
    if ((p & std::filesystem::perms::sticky_bit) != std::filesystem::perms::none) {
        mode |= 01000;
    }

    std::ostringstream oss;
    oss << std::oct << std::setfill('0') << std::setw(4) << mode;
    return oss.str();
}

// ============================================================================
// File type formatting
// ============================================================================

[[nodiscard]] inline char format_file_type_char(std::filesystem::file_type ft) {
    switch (ft) {
    case std::filesystem::file_type::regular:
        return '-';
    case std::filesystem::file_type::directory:
        return 'd';
    case std::filesystem::file_type::symlink:
        return 'l';
    case std::filesystem::file_type::block:
        return 'b';
    case std::filesystem::file_type::character:
        return 'c';
    case std::filesystem::file_type::fifo:
        return 'p';
    case std::filesystem::file_type::socket:
        return 's';
    default:
        return '?';
    }
}

[[nodiscard]] inline std::string_view format_file_type_name(std::filesystem::file_type ft) {
    switch (ft) {
    case std::filesystem::file_type::regular:
        return "regular";
    case std::filesystem::file_type::directory:
        return "directory";
    case std::filesystem::file_type::symlink:
        return "symlink";
    case std::filesystem::file_type::block:
        return "block";
    case std::filesystem::file_type::character:
        return "character";
    case std::filesystem::file_type::fifo:
        return "fifo";
    case std::filesystem::file_type::socket:
        return "socket";
    case std::filesystem::file_type::not_found:
        return "not_found";
    case std::filesystem::file_type::none:
        return "none";
    default:
        return "unknown";
    }
}

// ============================================================================
// Size literals
// ============================================================================

namespace size_literals {
constexpr std::uintmax_t operator""_B(unsigned long long v) {
    return v;
}
constexpr std::uintmax_t operator""_KB(unsigned long long v) {
    return v * 1000;
}
constexpr std::uintmax_t operator""_MB(unsigned long long v) {
    return v * 1000 * 1000;
}
constexpr std::uintmax_t operator""_GB(unsigned long long v) {
    return v * 1000 * 1000 * 1000;
}
constexpr std::uintmax_t operator""_TB(unsigned long long v) {
    return v * 1000ULL * 1000 * 1000 * 1000;
}
constexpr std::uintmax_t operator""_KiB(unsigned long long v) {
    return v * 1024;
}
constexpr std::uintmax_t operator""_MiB(unsigned long long v) {
    return v * 1024 * 1024;
}
constexpr std::uintmax_t operator""_GiB(unsigned long long v) {
    return v * 1024 * 1024 * 1024;
}
constexpr std::uintmax_t operator""_TiB(unsigned long long v) {
    return v * 1024ULL * 1024 * 1024 * 1024;
}
} // namespace size_literals

} // namespace frappe

#endif // FRAPPE_FORMAT_UTILS_HPP
