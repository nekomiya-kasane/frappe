add_executable(example_basic basic.cpp)
target_link_libraries(example_basic PRIVATE frappe)

add_executable(example_glob glob_example.cpp)
target_link_libraries(example_glob PRIVATE frappe)

add_executable(example_regex regex_example.cpp)
target_link_libraries(example_regex PRIVATE frappe)

add_executable(example_comprehensive comprehensive_example.cpp)
target_link_libraries(example_comprehensive PRIVATE frappe)

add_executable(example_pipeline pipeline_example.cpp)
target_link_libraries(example_pipeline PRIVATE frappe)

add_executable(example_entry_pipeline entry_pipeline_example.cpp)
target_link_libraries(example_entry_pipeline PRIVATE frappe)

add_executable(example_rpp rpp_example.cpp)
target_link_libraries(example_rpp PRIVATE frappe)
