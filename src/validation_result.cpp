#include "gamedataguard/validation_result.hpp"
#include <algorithm>

namespace gamedataguard {

int ValidationResult::error_count() const {
    int n = 0;
    for (const auto& d : diagnostics)
        if (d.severity == Severity::Error) ++n;
    return n;
}

int ValidationResult::warning_count() const {
    int n = 0;
    for (const auto& d : diagnostics)
        if (d.severity == Severity::Warning) ++n;
    return n;
}

int ValidationResult::info_count() const {
    int n = 0;
    for (const auto& d : diagnostics)
        if (d.severity == Severity::Info) ++n;
    return n;
}

bool ValidationResult::has_errors() const {
    return error_count() > 0;
}

bool ValidationResult::has_warnings() const {
    return warning_count() > 0;
}

void ValidationResult::sort() {
    std::stable_sort(diagnostics.begin(), diagnostics.end());
}

} // namespace gamedataguard
