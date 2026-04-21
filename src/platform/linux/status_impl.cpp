#ifdef FRAPPE_PLATFORM_LINUX

#include "frappe/status.hpp"

#include <limits.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

namespace frappe::detail {

    result<file_time_type> creation_time_impl(const path &p) noexcept {
        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

#ifdef ___linux_
#if defined(st_birthtime)
        auto duration = std::chrono::seconds(st.st_birthtime) + std::chrono::nanoseconds(st.st_birthtimensec);
        return file_time_type(std::chrono::duration_cast<file_time_type::duration>(duration));
#else
        return file_time_type(std::chrono::duration_cast<file_time_type::duration>(
            std::chrono::seconds(st.st_ctime) + std::chrono::nanoseconds(st.st_ctim.tv_nsec)));
#endif
#else
        return file_time_type(std::chrono::duration_cast<file_time_type::duration>(std::chrono::seconds(st.st_ctime)));
#endif
    }

    result<file_time_type> last_access_time_impl(const path &p) noexcept {
        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

#ifdef ___linux_
        auto duration = std::chrono::seconds(st.st_atime) + std::chrono::nanoseconds(st.st_atim.tv_nsec);
        return file_time_type(std::chrono::duration_cast<file_time_type::duration>(duration));
#else
        return file_time_type(std::chrono::duration_cast<file_time_type::duration>(std::chrono::seconds(st.st_atime)));
#endif
    }

    result<file_id> get_file_id_impl(const path &p) noexcept {
        struct stat st;
        if (stat(p.c_str(), &st) != 0) {
            return std::unexpected(make_error(errno, std::system_category()));
        }

        return file_id{static_cast<std::uint64_t>(st.st_dev), static_cast<std::uint64_t>(st.st_ino)};
    }

    result<std::uintmax_t> max_hard_links_impl(const path &p) noexcept {
        long r = pathconf(p.c_str(), _PC_LINK_MAX);
        if (r == -1) {
            if (errno != 0) {
                return std::unexpected(make_error(errno, std::system_category()));
            }
            return std::uintmax_t{LINK_MAX};
        }

        return static_cast<std::uintmax_t>(r);
    }

} // namespace frappe::detail

#endif // FRAPPE_PLATFORM_LINUX
