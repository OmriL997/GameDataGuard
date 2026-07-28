add_executable(gamedataguard_tests
    cli_tests.cpp
    data_loader_tests.cpp
    packer_tests.cpp
    quest_graph_tests.cpp
    validator_tests.cpp
)

target_link_libraries(gamedataguard_tests PRIVATE
    gamedataguard_core
    Catch2::Catch2WithMain
)

# Provide path to sample data as a compile-time definition
target_compile_definitions(gamedataguard_tests PRIVATE
    GDG_SAMPLE_DATA_DIR="${CMAKE_SOURCE_DIR}/sample_data"
)

gdg_set_warnings(gamedataguard_tests)
gdg_set_sanitizers(gamedataguard_tests)

include(CTest)
include(Catch)
catch_discover_tests(gamedataguard_tests)
