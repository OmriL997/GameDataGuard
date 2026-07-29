#include <catch2/catch_test_macros.hpp>
#include "gamedataguard/cli.hpp"

using namespace gamedataguard;

TEST_CASE("CLI parse_args: no args shows help", "[cli]") {
    auto result = parse_args({});
    REQUIRE(result.ok);
    REQUIRE(result.args.command == Command::Help);
}

TEST_CASE("CLI parse_args: help command", "[cli]") {
    auto r1 = parse_args({"help"});
    REQUIRE(r1.ok);
    REQUIRE(r1.args.command == Command::Help);

    auto r2 = parse_args({"--help"});
    REQUIRE(r2.ok);
    REQUIRE(r2.args.command == Command::Help);

    auto r3 = parse_args({"-h"});
    REQUIRE(r3.ok);
    REQUIRE(r3.args.command == Command::Help);
}

TEST_CASE("CLI parse_args: validate command", "[cli]") {
    auto r = parse_args({"validate", "some/dir"});
    REQUIRE(r.ok);
    REQUIRE(r.args.command == Command::Validate);
    REQUIRE(r.args.data_directory == "some/dir");
    REQUIRE_FALSE(r.args.report_path.has_value());
    REQUIRE_FALSE(r.args.warnings_as_errors);
}

TEST_CASE("CLI parse_args: validate with options", "[cli]") {
    auto r = parse_args({"validate", "some/dir", "--report", "out.json", "--warnings-as-errors"});
    REQUIRE(r.ok);
    REQUIRE(r.args.command == Command::Validate);
    REQUIRE(r.args.data_directory == "some/dir");
    REQUIRE(r.args.report_path.has_value());
    REQUIRE(r.args.report_path.value() == "out.json");
    REQUIRE(r.args.warnings_as_errors);
}

TEST_CASE("CLI parse_args: validate missing data_directory", "[cli]") {
    auto r = parse_args({"validate"});
    REQUIRE_FALSE(r.ok);
    REQUIRE_FALSE(r.error_message.empty());
}

TEST_CASE("CLI parse_args: build command", "[cli]") {
    auto r = parse_args({"build", "data/", "output.gdgpack"});
    REQUIRE(r.ok);
    REQUIRE(r.args.command == Command::Build);
    REQUIRE(r.args.data_directory == "data/");
    REQUIRE(r.args.output_pack == "output.gdgpack");
    REQUIRE_FALSE(r.args.warnings_as_errors);
}

TEST_CASE("CLI parse_args: build with options", "[cli]") {
    auto r = parse_args({"build", "data/", "output.gdgpack", "--report", "rep.json", "--warnings-as-errors"});
    REQUIRE(r.ok);
    REQUIRE(r.args.command == Command::Build);
    REQUIRE(r.args.report_path.has_value());
    REQUIRE(r.args.report_path.value() == "rep.json");
    REQUIRE(r.args.warnings_as_errors);
}

TEST_CASE("CLI parse_args: build missing output_pack", "[cli]") {
    auto r = parse_args({"build", "data/"});
    REQUIRE_FALSE(r.ok);
    REQUIRE_FALSE(r.error_message.empty());
}

TEST_CASE("CLI parse_args: unknown command", "[cli]") {
    auto r = parse_args({"unknown_command"});
    REQUIRE_FALSE(r.ok);
    REQUIRE_FALSE(r.error_message.empty());
}

TEST_CASE("CLI parse_args: unknown option in validate", "[cli]") {
    auto r = parse_args({"validate", "data/", "--nonexistent"});
    REQUIRE_FALSE(r.ok);
}

TEST_CASE("CLI parse_args: --report missing path argument", "[cli]") {
    auto r = parse_args({"validate", "data/", "--report"});
    REQUIRE_FALSE(r.ok);
}
