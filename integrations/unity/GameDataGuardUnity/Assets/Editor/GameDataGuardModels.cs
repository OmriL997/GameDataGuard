// GameDataGuardModels.cs
//
// Serialisable C# models that mirror the JSON schema produced by
// gamedataguard_unity.dll via gdg_validate_directory_utf8().
//
// Schema contract (produced by the native bridge):
//   {
//     "status":       "passed" | "validation_failed" | "tool_error",
//     "errorCount":   <integer>,
//     "warningCount": <integer>,
//     "message":      "<string; non-empty only for tool_error>",
//     "diagnostics":  [
//       {
//         "severity": "error" | "warning" | "info",
//         "code":     "GDGxxx",
//         "file":     "<filename>",
//         "path":     "<JSON pointer or empty string>",
//         "message":  "<diagnostic message>"
//       }
//     ]
//   }
//
// Unity's JsonUtility is used for deserialisation.  All optional string fields
// are represented as non-null strings (empty string when absent) to remain
// compatible with JsonUtility, which does not handle JSON null values.

using System;
using UnityEngine;

namespace GameDataGuard.Editor
{
    /// <summary>Severity of a single validation diagnostic.</summary>
    public enum DiagnosticSeverity
    {
        Error,
        Warning,
        Info,
        Unknown
    }

    /// <summary>A single diagnostic entry returned by the native validation layer.</summary>
    [Serializable]
    public class ValidationDiagnostic
    {
        /// <summary>Lower-case severity: "error", "warning", or "info".</summary>
        public string severity = "";

        /// <summary>Stable diagnostic code, e.g. "GDG006".</summary>
        public string code = "";

        /// <summary>Relative filename that contains the problem, e.g. "quests.json".</summary>
        public string file = "";

        /// <summary>JSON pointer to the offending value, or empty string if not applicable.</summary>
        public string path = "";

        /// <summary>Human-readable diagnostic message.</summary>
        public string message = "";

        /// <summary>Typed severity derived from the lower-case severity string.</summary>
        public DiagnosticSeverity Severity
        {
            get
            {
                switch (severity)
                {
                    case "error":   return DiagnosticSeverity.Error;
                    case "warning": return DiagnosticSeverity.Warning;
                    case "info":    return DiagnosticSeverity.Info;
                    default:        return DiagnosticSeverity.Unknown;
                }
            }
        }
    }

    /// <summary>
    /// The full result returned by a native GameDataGuard validation call.
    /// Deserialised from the JSON string produced by gamedataguard_unity.dll.
    /// </summary>
    [Serializable]
    public class ValidationResult
    {
        /// <summary>"passed", "validation_failed", or "tool_error".</summary>
        public string status = "";

        /// <summary>Number of error-severity diagnostics.</summary>
        public int errorCount;

        /// <summary>Number of warning-severity diagnostics.</summary>
        public int warningCount;

        /// <summary>
        /// Human-readable error description when status is "tool_error".
        /// Empty string for "passed" or "validation_failed".
        /// </summary>
        public string message = "";

        /// <summary>Sorted list of structured diagnostics.</summary>
        public ValidationDiagnostic[] diagnostics = Array.Empty<ValidationDiagnostic>();

        // ----------------------------------------------------------------
        // Convenience properties
        // ----------------------------------------------------------------

        public bool IsPassed           => status == "passed";
        public bool IsValidationFailed => status == "validation_failed";
        public bool IsToolError        => status == "tool_error";
    }
}
