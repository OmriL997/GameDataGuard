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

# ---------------------------------------------------------------------------
# Unity bridge tests (Windows only; requires GDG_BUILD_UNITY_PLUGIN=ON)
# ---------------------------------------------------------------------------
if(GDG_BUILD_UNITY_PLUGIN)
    add_executable(gamedataguard_unity_tests
        unity_bridge_tests.cpp
    )

    target_include_directories(gamedataguard_unity_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/integrations/unity/native
    )

    target_link_libraries(gamedataguard_unity_tests PRIVATE
        gamedataguard_unity
        Catch2::Catch2WithMain
        nlohmann_json::nlohmann_json
    )

    target_compile_definitions(gamedataguard_unity_tests PRIVATE
        GDG_SAMPLE_DATA_DIR="${CMAKE_SOURCE_DIR}/sample_data"
    )

    # Co-locate the test exe with the DLL so the Windows loader finds it.
    set_target_properties(gamedataguard_unity_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:gamedataguard_unity>
    )

    gdg_set_warnings(gamedataguard_unity_tests)

    # Use plain add_test rather than catch_discover_tests to avoid the DLL
    # discovery timing issue on Windows multi-config builds.
    add_test(
        NAME    gamedataguard_unity_tests
        COMMAND gamedataguard_unity_tests
        WORKING_DIRECTORY $<TARGET_FILE_DIR:gamedataguard_unity_tests>
    )
endif()
