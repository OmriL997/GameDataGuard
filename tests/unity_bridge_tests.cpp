// unity_bridge_tests.cpp
//
// Catch2 tests for the gamedataguard_unity C ABI bridge.
//
// These tests call the public C functions directly (via the import library)
// to verify the contract without requiring Unity to be running.
//
// What is NOT tested here:
//   - Core validation logic (covered by validator_tests.cpp)
//   - Data loading edge cases (covered by data_loader_tests.cpp)
//   - Quest graph analysis (covered by quest_graph_tests.cpp)
//
// What IS tested here:
//   - The C ABI surface: status codes, JSON schema, memory contract
//   - End-to-end integration through the bridge for the four canonical cases

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "gamedataguard_unity.h"

#include <cstdint>
#include <string>

using json = nlohmann::json;

// -----------------------------------------------------------------------
// Helper: invoke the bridge, parse the JSON, free the native buffer,
// and return a struct with the status code and parsed json object.
// -----------------------------------------------------------------------
struct BridgeResult {
    std::int32_t status_code;
    json         parsed;
};

static BridgeResult call_validate(const char* path)
{
    char* raw = nullptr;
    std::int32_t code = gdg_validate_directory_utf8(path, &raw);
    REQUIRE(raw != nullptr);
    std::string json_str(raw);
    gdg_free_string(raw);
    return BridgeResult{code, json::parse(json_str)};
}

// Canonical sample-data paths injected at compile time by CMake.
static const std::string kValidDir   = std::string(GDG_SAMPLE_DATA_DIR) + "/valid";
static const std::string kMissRef    = std::string(GDG_SAMPLE_DATA_DIR) + "/invalid/missing_reference";
static const std::string kQuestCycle = std::string(GDG_SAMPLE_DATA_DIR) + "/invalid/quest_cycle";

// -----------------------------------------------------------------------
// 1. Valid sample directory passes
// -----------------------------------------------------------------------
TEST_CASE("Bridge: valid sample directory returns status 0 and passed",
          "[unity_bridge]")
{
    auto [code, j] = call_validate(kValidDir.c_str());
    CHECK(code == 0);
    CHECK(j["status"] == "passed");
    CHECK(j["errorCount"] == 0);
}

// -----------------------------------------------------------------------
// 2. Missing-reference directory returns status 1 and GDG006
// -----------------------------------------------------------------------
TEST_CASE("Bridge: missing-reference directory returns status 1 and GDG006",
          "[unity_bridge]")
{
    auto [code, j] = call_validate(kMissRef.c_str());
    CHECK(code == 1);
    CHECK(j["status"] == "validation_failed");
    CHECK(j["errorCount"].get<int>() > 0);

    bool found_gdg006 = false;
    for (const auto& d : j["diagnostics"])
        if (d["code"] == "GDG006") { found_gdg006 = true; break; }
    CHECK(found_gdg006);
}

// -----------------------------------------------------------------------
// 3. Quest-cycle directory returns status 1 and GDG013
// -----------------------------------------------------------------------
TEST_CASE("Bridge: quest-cycle directory returns status 1 and GDG013",
          "[unity_bridge]")
{
    auto [code, j] = call_validate(kQuestCycle.c_str());
    CHECK(code == 1);
    CHECK(j["status"] == "validation_failed");

    bool found_gdg013 = false;
    for (const auto& d : j["diagnostics"])
        if (d["code"] == "GDG013") { found_gdg013 = true; break; }
    CHECK(found_gdg013);
}

// -----------------------------------------------------------------------
// 4. Nonexistent directory returns status 2 (tool error)
// -----------------------------------------------------------------------
TEST_CASE("Bridge: nonexistent directory returns status 2",
          "[unity_bridge]")
{
    auto [code, j] = call_validate(
        "C:/this_path_does_not_exist_gamedataguard_test_12345");
    CHECK(code == 2);
    CHECK(j["status"] == "tool_error");
    CHECK(!j["message"].get<std::string>().empty());
}

// -----------------------------------------------------------------------
// 5. Null input pointer returns status 2 (tool error)
// -----------------------------------------------------------------------
TEST_CASE("Bridge: null directory path returns status 2", "[unity_bridge]")
{
    char* raw = nullptr;
    std::int32_t code = gdg_validate_directory_utf8(nullptr, &raw);
    CHECK(code == 2);
    REQUIRE(raw != nullptr);
    json j = json::parse(raw);
    gdg_free_string(raw);
    CHECK(j["status"] == "tool_error");
    CHECK(!j["message"].get<std::string>().empty());
}

// -----------------------------------------------------------------------
// 6. Correct JSON structure — all required top-level fields present
// -----------------------------------------------------------------------
TEST_CASE("Bridge: result JSON contains all required fields",
          "[unity_bridge]")
{
    auto [code, j] = call_validate(kValidDir.c_str());
    CHECK(j.contains("status"));
    CHECK(j.contains("errorCount"));
    CHECK(j.contains("warningCount"));
    CHECK(j.contains("message"));
    CHECK(j.contains("diagnostics"));
    CHECK(j["diagnostics"].is_array());
    CHECK(j["message"].is_string());
}

// -----------------------------------------------------------------------
// 7. Correct error and warning counts
// -----------------------------------------------------------------------
TEST_CASE("Bridge: error and warning counts match diagnostics",
          "[unity_bridge]")
{
    // Valid data: zero errors, zero warnings.
    auto [c1, j1] = call_validate(kValidDir.c_str());
    CHECK(j1["errorCount"] == 0);
    CHECK(j1["warningCount"] == 0);

    // Missing-reference data: at least one error.
    auto [c2, j2] = call_validate(kMissRef.c_str());
    int reported_errors = j2["errorCount"].get<int>();
    CHECK(reported_errors > 0);

    // Count errors directly from the diagnostics array for consistency.
    int counted_errors = 0;
    for (const auto& d : j2["diagnostics"])
        if (d["severity"] == "error") ++counted_errors;
    CHECK(reported_errors == counted_errors);
}

// -----------------------------------------------------------------------
// 8. Diagnostic fields are present and severity is lower-case
// -----------------------------------------------------------------------
TEST_CASE("Bridge: diagnostic entries have correct fields and lower-case severity",
          "[unity_bridge]")
{
    auto [code, j] = call_validate(kMissRef.c_str());
    REQUIRE(!j["diagnostics"].empty());

    for (const auto& d : j["diagnostics"]) {
        CHECK(d.contains("severity"));
        CHECK(d.contains("code"));
        CHECK(d.contains("file"));
        CHECK(d.contains("path"));   // always present; empty string if no path
        CHECK(d.contains("message"));
        CHECK(d["path"].is_string());

        std::string sev = d["severity"].get<std::string>();
        bool valid_sev  = (sev == "error" || sev == "warning" || sev == "info");
        CHECK(valid_sev);
    }
}

// -----------------------------------------------------------------------
// 9. gdg_free_string(nullptr) is a safe no-op
// -----------------------------------------------------------------------
TEST_CASE("Bridge: gdg_free_string(nullptr) does not crash", "[unity_bridge]")
{
    gdg_free_string(nullptr);
    // No crash = pass.
}

// -----------------------------------------------------------------------
// 10. Repeated calls do not leak state or corrupt results
// -----------------------------------------------------------------------
TEST_CASE("Bridge: repeated calls produce consistent results",
          "[unity_bridge]")
{
    for (int i = 0; i < 5; ++i) {
        auto [code, j] = call_validate(kValidDir.c_str());
        CHECK(code == 0);
        CHECK(j["status"] == "passed");
        CHECK(j["errorCount"] == 0);
    }

    for (int i = 0; i < 5; ++i) {
        auto [code, j] = call_validate(kMissRef.c_str());
        CHECK(code == 1);
        CHECK(j["status"] == "validation_failed");
    }
}
