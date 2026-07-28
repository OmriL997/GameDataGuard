#pragma once

#include <string>
#include <vector>
#include <optional>

namespace gamedataguard {

enum class Command {
    Help,
    Validate,
    Build
};

struct CliArgs {
    Command command{Command::Help};
    std::string data_directory;
    std::string output_pack;
    std::optional<std::string> report_path;
    bool warnings_as_errors{false};
};

// Returns parsed args or an error message.
// On error, caller should print the message and return exit code 2.
struct ParseResult {
    bool ok{false};
    std::string error_message;
    CliArgs args;
};

ParseResult parse_args(const std::vector<std::string>& argv);
void print_help();

} // namespace gamedataguard
