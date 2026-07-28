#pragma once

#include <string>
#include <vector>
#include <optional>

namespace gamedataguard {

enum class Severity {
    Info,
    Warning,
    Error
};

struct Diagnostic {
    Severity severity;
    std::string code;       // e.g. "GDG006"
    std::string file;       // relative filename
    std::optional<std::string> path; // JSON pointer or field path
    std::string message;

    // Comparison for deterministic sorting:
    // 1. severity (Error < Warning < Info)
    // 2. file
    // 3. path
    // 4. code
    bool operator<(const Diagnostic& other) const;
};

std::string severity_to_string(Severity s);

} // namespace gamedataguard
