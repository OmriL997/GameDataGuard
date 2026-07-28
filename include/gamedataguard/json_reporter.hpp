#pragma once

#include "validation_result.hpp"
#include <filesystem>
#include <string>

namespace gamedataguard {

// Writes a JSON report to report_path.
// Returns true on success, false on failure.
bool write_json_report(
    const std::filesystem::path& report_path,
    const std::string& data_directory,
    const ValidationResult& result,
    bool pack_built);

} // namespace gamedataguard
