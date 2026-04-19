# frappe — Cross-platform filesystem library (COCA workspace build)
# Provides: frappe (SHARED), frappe::frappe
# Dependencies: COCA::pcre2, COCA::stdexec, COCA::range_v3

# ── Guard: skip if required COCA:: deps are not available ────────────
foreach(_dep IN ITEMS COCA::stdexec COCA::range_v3 COCA::pcre2)
    if(NOT TARGET ${_dep})
        message(STATUS "[frappe] Skipping — ${_dep} not available")
        add_library(frappe INTERFACE)
        add_library(frappe::frappe ALIAS frappe)
        return()
    endif()
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(FRAPPE_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FRAPPE_PLATFORM "linux")
else()
    set(FRAPPE_PLATFORM "windows")
endif()

set(FRAPPE_PUBLIC_HEADERS
    include/frappe/frappe.hpp
    include/frappe/version.hpp
    include/frappe/path.hpp
    include/frappe/glob.hpp
    include/frappe/regex.hpp
    include/frappe/error.hpp
    include/frappe/find.hpp
    include/frappe/status.hpp
    include/frappe/permissions.hpp
    include/frappe/attributes.hpp
    include/frappe/filesystem.hpp
    include/frappe/disk.hpp
    include/frappe/async.hpp
    include/frappe/file_lock.hpp
    include/frappe/watcher.hpp
    include/frappe/entry.hpp
    include/frappe/projections.hpp
    include/frappe/filters.hpp
    include/frappe/sorters.hpp
    include/frappe/aggregators.hpp
    include/frappe/format_utils.hpp
    include/frappe/formatters.hpp
    include/frappe/transforms.hpp
    include/frappe/mmap.hpp
    include/frappe/traversal.hpp
    include/frappe/entry_adapters.hpp
)

set(RPP_PUBLIC_HEADERS
    include/rpp/rpp.hpp
    include/rpp/concepts.hpp
    include/rpp/pipeable.hpp
    include/rpp/comparator.hpp
    include/rpp/filter.hpp
    include/rpp/filters_ext.hpp
    include/rpp/combine.hpp
    include/rpp/sort.hpp
    include/rpp/select.hpp
    include/rpp/distinct.hpp
    include/rpp/aggregate.hpp
    include/rpp/aggregate_ext.hpp
    include/rpp/transform.hpp
    include/rpp/join.hpp
    include/rpp/set_ops.hpp
    include/rpp/window.hpp
    include/rpp/extras.hpp
)

set(FRAPPE_SOURCES
    src/version.cpp
    src/path.cpp
    src/glob.cpp
    src/regex.cpp
    src/error.cpp
    src/find.cpp
    src/status.cpp
    src/permissions.cpp
    src/attributes.cpp
    src/filesystem.cpp
    src/disk.cpp
    src/file_lock.cpp
    src/watcher.cpp
    src/entry.cpp
    src/mmap.cpp
    src/traversal.cpp
)

if(FRAPPE_PLATFORM STREQUAL "windows")
    list(APPEND FRAPPE_SOURCES
        src/platform/windows/path_impl.cpp
        src/platform/windows/status_impl.cpp
        src/platform/windows/permissions_impl.cpp
        src/platform/windows/attributes_impl.cpp
        src/platform/windows/filesystem_impl.cpp
        src/platform/windows/disk_impl.cpp
        src/platform/windows/file_lock_impl.cpp
        src/platform/windows/watcher_impl.cpp
    )
elseif(FRAPPE_PLATFORM STREQUAL "linux")
    list(APPEND FRAPPE_SOURCES
        src/platform/linux/path_impl.cpp
        src/platform/linux/status_impl.cpp
        src/platform/linux/permissions_impl.cpp
        src/platform/linux/attributes_impl.cpp
        src/platform/linux/filesystem_impl.cpp
        src/platform/linux/disk_impl.cpp
        src/platform/linux/file_lock_impl.cpp
        src/platform/linux/watcher_impl.cpp
    )
elseif(FRAPPE_PLATFORM STREQUAL "macos")
    list(APPEND FRAPPE_SOURCES
        src/platform/macos/path_impl.cpp
        src/platform/macos/status_impl.cpp
        src/platform/macos/permissions_impl.cpp
        src/platform/macos/attributes_impl.cpp
        src/platform/macos/filesystem_impl.cpp
        src/platform/macos/disk_impl.cpp
        src/platform/macos/file_lock_impl.cpp
        src/platform/macos/watcher_impl.cpp
    )
endif()

add_library(frappe SHARED ${FRAPPE_SOURCES})
add_library(frappe::frappe ALIAS frappe)

set_target_properties(frappe PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(frappe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(frappe
    PUBLIC
        COCA::stdexec
        COCA::range_v3
    PRIVATE
        COCA::pcre2
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(frappe PRIVATE shell32 ole32 uuid)
endif()

target_compile_definitions(frappe
    PRIVATE
        FRAPPE_BUILD_INTERNAL
        FRAPPE_VERSION_MAJOR=0
        FRAPPE_VERSION_MINOR=1
        FRAPPE_VERSION_PATCH=0
        FRAPPE_PLATFORM_WINDOWS=$<BOOL:$<STREQUAL:${FRAPPE_PLATFORM},windows>>
        FRAPPE_PLATFORM_LINUX=$<BOOL:$<STREQUAL:${FRAPPE_PLATFORM},linux>>
)

if(MSVC)
    target_compile_options(frappe PUBLIC /W4 /utf-8)
else()
    target_compile_options(frappe PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Tests ───────────────────────────────────────────────────────────────
option(FRAPPE_BUILD_TESTS "Build frappe tests" ON)
if(FRAPPE_BUILD_TESTS AND TARGET COCA::googletest)
    enable_testing()
    add_subdirectory(tests)
endif()


