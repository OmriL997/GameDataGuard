#include "gamedataguard/json_reporter.hpp"
#include "gamedataguard/diagnostic.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

namespace gamedataguard {

using json = nlohmann::json;

bool write_json_report(
    const std::filesystem::path& report_path,
    const std::string& data_directory,
    const ValidationResult& result,
    bool pack_built)
{
    int errors   = result.error_count();
    int warnings = result.warning_count();
    int infos    = result.info_count();

    std::string status;
    if (errors > 0)
        status = "failed";
    else if (pack_built)
        status = "passed";
    else
        status = "validated";

    json diag_array = json::array();
    for (const auto& d : result.diagnostics) {
        json entry;
        entry["severity"] = severity_to_string(d.severity);
        entry["code"]     = d.code;
        entry["file"]     = d.file;
        if (d.path.has_value())
            entry["path"] = d.path.value();
        else
            entry["path"] = nullptr;
        entry["message"]  = d.message;
        diag_array.push_back(std::move(entry));
    }

    json report;
    report["tool"]           = "GameDataGuard";
    report["tool_version"]   = "1.0.0";
    report["data_directory"] = data_directory;
    report["status"]         = status;
    report["summary"] = {
        {"errors",   errors},
        {"warnings", warnings},
        {"infos",    infos}
    };
    report["diagnostics"] = std::move(diag_array);

    // Write to temp file, then rename
    auto tmp_path = report_path;
    tmp_path += ".tmp";

    {
        std::ofstream f(tmp_path, std::ios::binary);
        if (!f.is_open()) return false;
        f << report.dump(2);
        if (!f.good()) {
            std::filesystem::remove(tmp_path);
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmp_path, report_path, ec);
    if (ec) {
        std::filesystem::remove(tmp_path, ec);
        return false;
    }
    return true;
}

} // namespace gamedataguard
