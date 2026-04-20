#include "frappe/path.hpp"

#include <cstdlib>

namespace frappe {

namespace detail {
result<path> home_path_impl() noexcept;
result<path> executable_path_impl() noexcept;
result<path> app_data_path_impl() noexcept;
result<path> desktop_path_impl() noexcept;
result<path> documents_path_impl() noexcept;
result<path> downloads_path_impl() noexcept;
} // namespace detail

result<path> home_path() noexcept {
    return detail::home_path_impl();
}

result<path> executable_path() noexcept {
    return detail::executable_path_impl();
}

result<path> app_data_path() noexcept {
    return detail::app_data_path_impl();
}

result<path> desktop_path() noexcept {
    return detail::desktop_path_impl();
}

result<path> documents_path() noexcept {
    return detail::documents_path_impl();
}

result<path> downloads_path() noexcept {
    return detail::downloads_path_impl();
}

result<path> resolve_path(const path &p) noexcept {
    std::string str = p.string();
    if (str.empty()) {
        return p;
    }

    if (str[0] == '~') {
        auto home_result = home_path();
        if (!home_result) {
            return std::unexpected(home_result.error());
        }
        if (str.size() == 1) {
            return *home_result;
        }
        if (str[1] == '/' || str[1] == '\\') {
            return *home_result / str.substr(2);
        }
    }

    std::string result_str;
    result_str.reserve(str.size());

    // @todo (Nekomiya) use some template engine
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '$') {
            if (i + 1 < str.size() && str[i + 1] == '{') {
                auto end = str.find('}', i + 2);
                if (end != std::string::npos) {
                    std::string var_name = str.substr(i + 2, end - i - 2);
                    const char *var_value = std::getenv(var_name.c_str());
                    if (var_value) {
                        result_str += var_value;
                    }
                    i = end;
                    continue;
                }
            } else if (i + 1 < str.size() && (std::isalnum(str[i + 1]) || str[i + 1] == '_')) {
                size_t end = i + 1;
                while (end < str.size() && (std::isalnum(str[end]) || str[end] == '_')) {
                    ++end;
                }
                std::string var_name = str.substr(i + 1, end - i - 1);
                const char *var_value = std::getenv(var_name.c_str());
                if (var_value) {
                    result_str += var_value;
                }
                i = end - 1;
                continue;
            }
        }
        result_str += str[i];
    }

    return path(result_str);
}

} // namespace frappe
