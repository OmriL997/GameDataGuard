#include "gamedataguard/cli.hpp"
#include <iostream>
#include <string>

namespace gamedataguard {

void print_help() {
    std::cout <<
        "GameDataGuard 1.0.0 - Game data validation and packaging tool\n"
        "\n"
        "USAGE:\n"
        "  gamedataguard validate <data_directory> [options]\n"
        "  gamedataguard build    <data_directory> <output_pack> [options]\n"
        "  gamedataguard help\n"
        "\n"
        "COMMANDS:\n"
        "  validate  Validate data files in <data_directory>.\n"
        "  build     Validate and, if valid, package data into <output_pack>.\n"
        "  help      Show this help message.\n"
        "\n"
        "OPTIONS:\n"
        "  --report <path>      Write a JSON report to <path>.\n"
        "  --warnings-as-errors Treat warnings as errors.\n"
        "  -h, --help           Show this help message.\n"
        "\n"
        "EXIT CODES:\n"
        "  0  Success\n"
        "  1  Validation failure\n"
        "  2  Usage error, file-access error, malformed JSON, or internal failure\n";
}

ParseResult parse_args(const std::vector<std::string>& argv) {
    ParseResult pr;

    if (argv.empty()) {
        pr.args.command = Command::Help;
        pr.ok = true;
        return pr;
    }

    const std::string& cmd = argv[0];

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        pr.args.command = Command::Help;
        pr.ok = true;
        return pr;
    }

    if (cmd == "validate") {
        pr.args.command = Command::Validate;
        if (argv.size() < 2) {
            pr.error_message = "validate: <data_directory> is required.";
            return pr;
        }
        pr.args.data_directory = argv[1];

        for (std::size_t i = 2; i < argv.size(); ++i) {
            if (argv[i] == "--report") {
                if (i + 1 >= argv.size()) {
                    pr.error_message = "--report requires a path argument.";
                    return pr;
                }
                pr.args.report_path = argv[++i];
            } else if (argv[i] == "--warnings-as-errors") {
                pr.args.warnings_as_errors = true;
            } else {
                pr.error_message = "Unknown option: " + argv[i];
                return pr;
            }
        }
        pr.ok = true;
        return pr;
    }

    if (cmd == "build") {
        pr.args.command = Command::Build;
        if (argv.size() < 3) {
            pr.error_message = "build: <data_directory> and <output_pack> are required.";
            return pr;
        }
        pr.args.data_directory = argv[1];
        pr.args.output_pack    = argv[2];

        for (std::size_t i = 3; i < argv.size(); ++i) {
            if (argv[i] == "--report") {
                if (i + 1 >= argv.size()) {
                    pr.error_message = "--report requires a path argument.";
                    return pr;
                }
                pr.args.report_path = argv[++i];
            } else if (argv[i] == "--warnings-as-errors") {
                pr.args.warnings_as_errors = true;
            } else {
                pr.error_message = "Unknown option: " + argv[i];
                return pr;
            }
        }
        pr.ok = true;
        return pr;
    }

    pr.error_message = "Unknown command: \"" + cmd + "\". Run 'gamedataguard help' for usage.";
    return pr;
}

} // namespace gamedataguard
