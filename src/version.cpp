#include "frappe/version.hpp"

#define FRAPPE_STRINGIFY_IMPL(x) #x
#define FRAPPE_STRINGIFY(x) FRAPPE_STRINGIFY_IMPL(x)

namespace frappe::version {

    static constexpr char version_str[] = FRAPPE_STRINGIFY(FRAPPE_VERSION_MAJOR) "." FRAPPE_STRINGIFY(
        FRAPPE_VERSION_MINOR) "." FRAPPE_STRINGIFY(FRAPPE_VERSION_PATCH);

    std::string_view string() noexcept {
        return version_str;
    }

} // namespace frappe::version
