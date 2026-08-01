// gamedataguard_unity.cpp
//
// Thin C ABI bridge between Unity (C# P/Invoke) and gamedataguard_core.
//
// Design decisions
// ----------------
// * All C++ objects and STL types stay behind this boundary.
// * Severity strings are lower-case ("error", "warning", "info") to follow
//   idiomatic JSON conventions.  The CLI reporter uses upper-case; the two
//   representations are independent and intentional.
// * The "path" field in each diagnostic is always present as a string.
//   An absent optional<string> is represented as an empty string so that
//   Unity's JsonUtility does not have to handle JSON null values.
// * Memory is allocated with new char[] and released with delete[].
//   gdg_free_string(nullptr) is safe because delete[] nullptr is a no-op.
// * All exceptions are caught at this boundary.  Nothing may propagate into
//   the C# caller.

#include "gamedataguard_unity.h"

#include "gamedataguard/data_loader.hpp"
#include "gamedataguard/diagnostic.hpp"
#include "gamedataguard/validation_result.hpp"
#include "gamedataguard/validator.hpp"

#include <nlohmann/json.hpp>

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <variant>

namespace {

using json = nlohmann::json;

// Lower-case severity string for the Unity bridge JSON.
std::string severity_to_lower(gamedataguard::Severity s)
{
    switch (s) {
        case gamedataguard::Severity::Error:   return "error";
        case gamedataguard::Severity::Warning: return "warning";
        case gamedataguard::Severity::Info:    return "info";
    }
    return "unknown";
}

// Heap-allocates a copy of str as a NUL-terminated char array.
// The caller owns the memory and must release it via gdg_free_string().
char* alloc_string(const std::string& str)
{
    char* buf = new char[str.size() + 1];
    std::memcpy(buf, str.c_str(), str.size() + 1);
    return buf;
}

// Builds the JSON string for a tool-level or internal error.
char* make_tool_error_json(const std::string& message)
{
    json j;
    j["status"]       = "tool_error";
    j["errorCount"]   = 0;
    j["warningCount"] = 0;
    j["message"]      = message;
    j["diagnostics"]  = json::array();
    return alloc_string(j.dump());
}

// Serialises a completed ValidationResult to the bridge JSON format.
char* make_result_json(const gamedataguard::ValidationResult& result)
{
    json diag_array = json::array();
    for (const auto& d : result.diagnostics) {
        json entry;
        entry["severity"] = severity_to_lower(d.severity);
        entry["code"]     = d.code;
        entry["file"]     = d.file;
        entry["path"]     = d.path.has_value() ? d.path.value() : "";
        entry["message"]  = d.message;
        diag_array.push_back(std::move(entry));
    }

    json j;
    j["status"]       = result.has_errors() ? "validation_failed" : "passed";
    j["errorCount"]   = result.error_count();
    j["warningCount"] = result.warning_count();
    j["message"]      = "";
    j["diagnostics"]  = std::move(diag_array);
    return alloc_string(j.dump());
}

} // anonymous namespace

extern "C" {

GDG_UNITY_API std::int32_t gdg_validate_directory_utf8(
    const char* directory_path_utf8,
    char**      result_json_utf8)
{
    if (result_json_utf8 == nullptr) {
        // Cannot write the result pointer — there is nothing safe to do.
        return 2;
    }

    try {
        if (directory_path_utf8 == nullptr) {
            *result_json_utf8 = make_tool_error_json(
                "Directory path must not be null.");
            return 2;
        }

        std::filesystem::path data_dir(directory_path_utf8);

        auto load_result = gamedataguard::load_game_data(data_dir);

        if (std::holds_alternative<gamedataguard::LoadError>(load_result)) {
            const auto& err = std::get<gamedataguard::LoadError>(load_result);
            *result_json_utf8 = make_tool_error_json(err.message);
            return 2;
        }

        const auto& success = std::get<gamedataguard::LoadSuccess>(load_result);
        gamedataguard::ValidationResult result =
            gamedataguard::validate(success.data);

        bool has_errors     = result.has_errors();
        *result_json_utf8   = make_result_json(result);
        return has_errors ? 1 : 0;
    }
    catch (const std::exception& ex) {
        *result_json_utf8 = make_tool_error_json(
            std::string("Unexpected exception: ") + ex.what());
        return 2;
    }
    catch (...) {
        *result_json_utf8 = make_tool_error_json("Unknown internal error.");
        return 2;
    }
}

GDG_UNITY_API void gdg_free_string(char* value)
{
    delete[] value;
}

} // extern "C"
