add_executable(frappe_tests
    test_version.cpp
    test_path.cpp
    test_glob.cpp
    test_regex.cpp
    test_find.cpp
    test_status.cpp
    test_permissions.cpp
    test_attributes.cpp
    test_filesystem.cpp
    test_disk.cpp
    test_async.cpp
    test_file_lock.cpp
    test_watcher.cpp
    test_mmap.cpp
    test_traversal.cpp
    test_filters.cpp
    test_sorters.cpp
    test_aggregators.cpp
    test_transforms.cpp
    test_projections.cpp
    test_entry.cpp
    test_formatters.cpp
    test_rpp.cpp
)

target_link_libraries(frappe_tests
    PRIVATE
        frappe::frappe
        COCA::googletest
)

set_target_properties(frappe_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/code/bin"
)

# Copy pcre2 DLL next to test executable
if(TARGET COCA::pcre2)
    get_target_property(_pcre2_loc COCA::pcre2 INTERFACE_LINK_LIBRARIES)
    get_filename_component(_pcre2_lib_dir "${_pcre2_loc}" DIRECTORY)
    get_filename_component(_pcre2_root "${_pcre2_lib_dir}" DIRECTORY)
    set(_pcre2_dll "${_pcre2_root}/bin/pcre2-8.dll")
    if(EXISTS "${_pcre2_dll}")
        add_custom_command(TARGET frappe_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_pcre2_dll}" "$<TARGET_FILE_DIR:frappe_tests>"
        )
    endif()
endif()

include(GoogleTest)
gtest_discover_tests(frappe_tests
    DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:frappe_tests>"
)
