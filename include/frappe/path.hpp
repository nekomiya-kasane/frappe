#ifndef FRAPPE_PATH_HPP
#define FRAPPE_PATH_HPP

#include "frappe/error.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace frappe {

using path = std::filesystem::path;
using path_view = std::string_view;

[[nodiscard]] FRAPPE_API result<path> home_path() noexcept;
[[nodiscard]] FRAPPE_API result<path> executable_path() noexcept;
[[nodiscard]] FRAPPE_API result<path> app_data_path() noexcept;
[[nodiscard]] FRAPPE_API result<path> desktop_path() noexcept;
[[nodiscard]] FRAPPE_API result<path> documents_path() noexcept;
[[nodiscard]] FRAPPE_API result<path> downloads_path() noexcept;
[[nodiscard]] FRAPPE_API result<path> resolve_path(const path &p) noexcept;

} // namespace frappe

#endif // FRAPPE_PATH_HPP
