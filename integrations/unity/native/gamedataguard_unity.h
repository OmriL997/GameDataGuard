#pragma once
#include <cstdint>

// Export/import macro for the Windows DLL.
// Define GDG_UNITY_EXPORTS when compiling the DLL itself;
// leave it undefined when consuming the DLL (e.g. tests, C# P/Invoke).
#ifdef _WIN32
    #ifdef GDG_UNITY_EXPORTS
        #define GDG_UNITY_API __declspec(dllexport)
    #else
        #define GDG_UNITY_API __declspec(dllimport)
    #endif
#else
    #define GDG_UNITY_API __attribute__((visibility("default")))
#endif

extern "C" {

// Validates the game-data directory located at directory_path_utf8.
//
// Return values:
//   0  Validation completed — no errors found.
//   1  Validation completed — one or more errors found.
//   2  Tool, input, loading, serialisation, or internal failure.
//
// On every non-null-out-pointer return, *result_json_utf8 is set to a
// newly heap-allocated, NUL-terminated UTF-8 JSON string.
// The caller must release it by passing it to gdg_free_string().
// *result_json_utf8 is never NULL on return when result_json_utf8 is non-null.
//
// JSON schema:
// {
//   "status":       "passed" | "validation_failed" | "tool_error",
//   "errorCount":   <integer>,
//   "warningCount": <integer>,
//   "message":      "<human-readable text for tool_error; empty string otherwise>",
//   "diagnostics":  [
//     {
//       "severity": "error" | "warning" | "info",
//       "code":     "GDGxxx",
//       "file":     "<filename>",
//       "path":     "<JSON pointer or empty string if not applicable>",
//       "message":  "<diagnostic message>"
//     }
//   ]
// }
//
// "severity" is lower-case (differs from the CLI reporter which uses upper-case).
// "path" is always present; it is an empty string when no JSON path is associated.
GDG_UNITY_API std::int32_t gdg_validate_directory_utf8(
    const char* directory_path_utf8,
    char**      result_json_utf8);

// Releases a string previously allocated by gdg_validate_directory_utf8.
// Passing NULL is safe (no-op).
GDG_UNITY_API void gdg_free_string(char* value);

} // extern "C"
