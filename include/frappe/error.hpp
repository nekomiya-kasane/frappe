#ifndef FRAPPE_ERROR_HPP
#define FRAPPE_ERROR_HPP

#include <stdexcept>
#include <string>
#include <system_error>
#include <filesystem>
#include <expected>

#include "frappe/exports.hpp"

namespace frappe {

    // Unified error type for expected results
    using error = std::error_code;

    // Result type alias using std::expected
    template<typename T>
    using result = std::expected<T, error>;

    // Void result type
    using void_result = std::expected<void, error>;

    // Helper to create error results
    inline error make_error(std::errc ec) {
        return std::make_error_code(ec);
    }

    inline error make_error(int code, const std::error_category& cat = std::system_category()) {
        return std::error_code(code, cat);
    }

    // Helper to throw on error
    template<typename T>
    T& throw_if_error(result<T>& res, const std::filesystem::path& p = {}) {
        if (!res) {
            if (p.empty()) {
                throw std::filesystem::filesystem_error("operation failed", res.error());
            }
            throw std::filesystem::filesystem_error("operation failed", p, res.error());
        }
        return *res;
    }

    template<typename T>
    const T& throw_if_error(const result<T>& res, const std::filesystem::path& p = {}) {
        if (!res) {
            if (p.empty()) {
                throw std::filesystem::filesystem_error("operation failed", res.error());
            }
            throw std::filesystem::filesystem_error("operation failed", p, res.error());
        }
        return *res;
    }

    inline void throw_if_error(const void_result& res, const std::filesystem::path& p = {}) {
        if (!res) {
            if (p.empty()) {
                throw std::filesystem::filesystem_error("operation failed", res.error());
            }
            throw std::filesystem::filesystem_error("operation failed", p, res.error());
        }
    }

    // Legacy exception classes (for backward compatibility)
    class filesystem_error : public std::filesystem::filesystem_error {
    public:
        using std::filesystem::filesystem_error::filesystem_error;
    };

    class path_error : public filesystem_error {
    public:
        path_error(const std::string& what_arg, error ec)
            : filesystem_error(what_arg, ec) {
        }

        path_error(const std::string& what_arg, const std::filesystem::path& p1, error ec)
            : filesystem_error(what_arg, p1, ec) {
        }
    };

    class regex_error : public std::runtime_error {
    public:
        explicit regex_error(const std::string& what_arg, int error_code = 0)
            : std::runtime_error(what_arg), _error_code(error_code) {}

        [[nodiscard]] int error_code() const noexcept { return _error_code; }

    private:
        int _error_code;
    };

    class glob_error : public std::runtime_error {
    public:
        explicit glob_error(const std::string& what_arg)
            : std::runtime_error(what_arg) {}
    };

} // namespace frappe

#endif // FRAPPE_ERROR_HPP
