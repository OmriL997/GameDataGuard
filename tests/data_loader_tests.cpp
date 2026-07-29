#include <catch2/catch_test_macros.hpp>
#include "gamedataguard/data_loader.hpp"
#include "test_helpers/temp_dir.hpp"
#include <filesystem>

using namespace gamedataguard;
namespace fs = std::filesystem;

TEST_CASE("DataLoader: valid data loads successfully", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadSuccess>(result));

    const auto& data = std::get<LoadSuccess>(result).data;
    REQUIRE(data.manifest.schema_version == 1);
    REQUIRE(data.npcs.size() == 1);
    REQUIRE(data.npcs[0].id == "npc_guide");
    REQUIRE(data.items.size() == 1);
    REQUIRE(data.quests.size() == 1);
    REQUIRE(data.localization.count("en") == 1);
}

TEST_CASE("DataLoader: missing manifest returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    // Don't write any files
    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
}

TEST_CASE("DataLoader: malformed JSON returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    // Overwrite npcs.json with invalid JSON
    dir.write("npcs.json", "[{invalid json}");

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
    const auto& err = std::get<LoadError>(result);
    REQUIRE(err.message.find("npcs.json") != std::string::npos);
}

TEST_CASE("DataLoader: npcs.json not an array returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("npcs.json", R"({"id": "npc_guide"})");

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
}

TEST_CASE("DataLoader: items.json malformed returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("items.json", "not json at all {{{}}}");

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
}

TEST_CASE("DataLoader: quests.json malformed returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", "[{]");

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
}

TEST_CASE("DataLoader: localization file malformed JSON returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("localization/en.json", "{bad json");

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
}

TEST_CASE("DataLoader: missing localization file is not a LoadError", "[data_loader]") {
    // Missing locale files are validation errors, not load errors
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    fs::remove(dir.path() / "localization" / "en.json");

    auto result = load_game_data(dir.path());
    // Should succeed loading (validation will catch the missing file)
    REQUIRE(std::holds_alternative<LoadSuccess>(result));
}

TEST_CASE("DataLoader: NPC missing required field returns LoadError", "[data_loader]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("npcs.json", R"([{"id": "npc_x", "name_key": "n", "max_hp": 10, "attack": 1}])");
    // missing defense

    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadError>(result));
}

TEST_CASE("DataLoader: full valid sample data loads", "[data_loader]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto result = load_game_data(sample / "valid");
    REQUIRE(std::holds_alternative<LoadSuccess>(result));

    const auto& data = std::get<LoadSuccess>(result).data;
    REQUIRE(data.npcs.size() == 3);
    REQUIRE(data.items.size() == 4);
    REQUIRE(data.quests.size() == 5);
    REQUIRE(data.localization.size() == 3);
}

TEST_CASE("DataLoader: malformed_json sample returns LoadError", "[data_loader]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto result = load_game_data(sample / "invalid" / "malformed_json");
    REQUIRE(std::holds_alternative<LoadError>(result));
}
