#ifndef FRAPPE_VERSION_HPP
#define FRAPPE_VERSION_HPP

#include "frappe/exports.hpp"

#include <cstdint>
#include <string>
#include <string_view>

#ifndef FRAPPE_VERSION_MAJOR
#define FRAPPE_VERSION_MAJOR 0
#endif
#ifndef FRAPPE_VERSION_MINOR
#define FRAPPE_VERSION_MINOR 1
#endif
#ifndef FRAPPE_VERSION_PATCH
#define FRAPPE_VERSION_PATCH 0
#endif

namespace frappe::version {

constexpr std::uint32_t major = FRAPPE_VERSION_MAJOR;
constexpr std::uint32_t minor = FRAPPE_VERSION_MINOR;
constexpr std::uint32_t patch = FRAPPE_VERSION_PATCH;

[[nodiscard]] FRAPPE_API std::string_view string() noexcept;

[[nodiscard]] constexpr std::uint32_t number() noexcept {
    return major * 10000 + minor * 100 + patch;
}

} // namespace frappe::version

#endif // FRAPPE_VERSION_HPP
