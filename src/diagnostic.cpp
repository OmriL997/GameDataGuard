#include "gamedataguard/diagnostic.hpp"

namespace gamedataguard {

namespace {
    int severity_rank(Severity s) {
        switch (s) {
            case Severity::Error:   return 0;
            case Severity::Warning: return 1;
            case Severity::Info:    return 2;
        }
        return 3;
    }
}

bool Diagnostic::operator<(const Diagnostic& other) const {
    int r1 = severity_rank(severity);
    int r2 = severity_rank(other.severity);
    if (r1 != r2) return r1 < r2;
    if (file != other.file) return file < other.file;

    // Compare optional paths: absent < present
    bool has_path = path.has_value();
    bool other_has_path = other.path.has_value();
    if (has_path != other_has_path) return !has_path;
    if (has_path && other_has_path && path.value() != other.path.value())
        return path.value() < other.path.value();

    return code < other.code;
}

std::string severity_to_string(Severity s) {
    switch (s) {
        case Severity::Error:   return "ERROR";
        case Severity::Warning: return "WARNING";
        case Severity::Info:    return "INFO";
    }
    return "UNKNOWN";
}

} // namespace gamedataguard
