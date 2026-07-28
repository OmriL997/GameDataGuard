#pragma once

#include "diagnostic.hpp"
#include <vector>

namespace gamedataguard {

struct ValidationResult {
    std::vector<Diagnostic> diagnostics;

    int error_count() const;
    int warning_count() const;
    int info_count() const;
    bool has_errors() const;
    bool has_warnings() const;

    // Sort diagnostics deterministically
    void sort();
};

} // namespace gamedataguard
