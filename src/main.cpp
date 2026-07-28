#include "gamedataguard/cli.hpp"
#include "gamedataguard/data_loader.hpp"
#include "gamedataguard/validator.hpp"
#include "gamedataguard/json_reporter.hpp"
#include "gamedataguard/packer.hpp"
#include "gamedataguard/diagnostic.hpp"
#include "gamedataguard/validation_result.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Print all diagnostics to stdout
void print_diagnostics(const gamedataguard::ValidationResult& result) {
    for (const auto& d : result.diagnostics) {
        std::cout << gamedataguard::severity_to_string(d.severity)
                  << " " << d.code;
        if (d.path.has_value()) {
            std::cout << " [" << d.file << ":" << d.path.value() << "]";
        } else {
            std::cout << " [" << d.file << "]";
        }
        std::cout << "\n" << d.message << "\n\n";
    }
}

// Print summary line
void print_summary(const gamedataguard::ValidationResult& result) {
    int errors   = result.error_count();
    int warnings = result.warning_count();

    if (errors == 0 && warnings == 0) {
        std::cout << "Validation passed: no errors, no warnings.\n";
    } else if (errors > 0) {
        std::cout << "Validation failed: "
                  << errors << (errors == 1 ? " error" : " errors")
                  << ", "
                  << warnings << (warnings == 1 ? " warning" : " warnings")
                  << ".\n";
    } else {
        std::cout << "Validation passed with "
                  << warnings << (warnings == 1 ? " warning" : " warnings")
                  << ".\n";
    }
}

int run_validate(const gamedataguard::CliArgs& args) {
    fs::path data_dir = args.data_directory;

    auto load_result = gamedataguard::load_game_data(data_dir);

    if (std::holds_alternative<gamedataguard::LoadError>(load_result)) {
        const auto& err = std::get<gamedataguard::LoadError>(load_result);
        std::cerr << "ERROR: " << err.message << "\n";
        return 2;
    }

    const auto& data = std::get<gamedataguard::LoadSuccess>(load_result).data;
    auto result = gamedataguard::validate(data);

    print_diagnostics(result);
    print_summary(result);

    if (args.report_path.has_value()) {
        bool ok = gamedataguard::write_json_report(
            args.report_path.value(), args.data_directory, result, false);
        if (!ok) {
            std::cerr << "ERROR: Failed to write report: " << args.report_path.value() << "\n";
            return 2;
        }
    }

    bool failed = result.has_errors() ||
                  (args.warnings_as_errors && result.has_warnings());
    return failed ? 1 : 0;
}

int run_build(const gamedataguard::CliArgs& args) {
    fs::path data_dir = args.data_directory;

    auto load_result = gamedataguard::load_game_data(data_dir);

    if (std::holds_alternative<gamedataguard::LoadError>(load_result)) {
        const auto& err = std::get<gamedataguard::LoadError>(load_result);
        std::cerr << "ERROR: " << err.message << "\n";
        return 2;
    }

    const auto& data = std::get<gamedataguard::LoadSuccess>(load_result).data;
    auto result = gamedataguard::validate(data);

    print_diagnostics(result);
    print_summary(result);

    bool validation_failed = result.has_errors() ||
                             (args.warnings_as_errors && result.has_warnings());

    if (validation_failed) {
        std::cout << "Pack not created due to validation failure.\n";
        if (args.report_path.has_value()) {
            gamedataguard::write_json_report(
                args.report_path.value(), args.data_directory, result, false);
        }
        return 1;
    }

    // Write pack
    auto pack_error = gamedataguard::write_pack(data, args.output_pack);
    if (pack_error.has_value()) {
        std::cerr << "ERROR: Failed to write pack: " << pack_error.value() << "\n";
        if (args.report_path.has_value()) {
            gamedataguard::write_json_report(
                args.report_path.value(), args.data_directory, result, false);
        }
        return 2;
    }

    std::cout << "Pack written: " << args.output_pack << "\n";

    if (args.report_path.has_value()) {
        bool ok = gamedataguard::write_json_report(
            args.report_path.value(), args.data_directory, result, true);
        if (!ok) {
            std::cerr << "ERROR: Failed to write report: " << args.report_path.value() << "\n";
            return 2;
        }
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);

    auto parsed = gamedataguard::parse_args(args);

    if (!parsed.ok) {
        std::cerr << "ERROR: " << parsed.error_message << "\n\n";
        gamedataguard::print_help();
        return 2;
    }

    switch (parsed.args.command) {
        case gamedataguard::Command::Help:
            gamedataguard::print_help();
            return 0;

        case gamedataguard::Command::Validate:
            return run_validate(parsed.args);

        case gamedataguard::Command::Build:
            return run_build(parsed.args);
    }

    return 2;
}
