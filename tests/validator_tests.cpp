#include <catch2/catch_test_macros.hpp>
#include "gamedataguard/validator.hpp"
#include "gamedataguard/data_loader.hpp"
#include "gamedataguard/diagnostic.hpp"
#include "test_helpers/temp_dir.hpp"
#include <filesystem>
#include <algorithm>

using namespace gamedataguard;
namespace fs = std::filesystem;

// Helper: find first diagnostic with given code
static const Diagnostic* find_diag(const ValidationResult& r, const std::string& code) {
    for (const auto& d : r.diagnostics)
        if (d.code == code) return &d;
    return nullptr;
}

// Helper: load and validate from temp dir
static ValidationResult load_validate(const test_helpers::TempDir& dir) {
    auto result = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadSuccess>(result));
    return validate(std::get<LoadSuccess>(result).data);
}

TEST_CASE("Validator: valid sample data passes", "[validator]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "valid");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto result = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE_FALSE(result.has_errors());
}

TEST_CASE("Validator: unsupported schema version", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("manifest.json", R"({
  "schema_version": 99,
  "required_locales": ["en"],
  "start_quest_ids": ["quest_intro"]
})");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG002") != nullptr);
}

TEST_CASE("Validator: empty required_locales", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("manifest.json", R"({
  "schema_version": 1,
  "required_locales": [],
  "start_quest_ids": ["quest_intro"]
})");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG004") != nullptr);
}

TEST_CASE("Validator: empty start_quest_ids", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("manifest.json", R"({
  "schema_version": 1,
  "required_locales": ["en"],
  "start_quest_ids": []
})");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG004") != nullptr);
}

TEST_CASE("Validator: duplicate NPC ID", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("npcs.json", R"([
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 100, "attack": 5, "defense": 10},
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 80,  "attack": 3, "defense": 5}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG005") != nullptr);
}

TEST_CASE("Validator: duplicate item ID", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("items.json", R"([
  {"id": "item_potion", "name_key": "item.potion.name", "description_key": "item.potion.desc",
   "buy_price": 50, "sell_price": 20},
  {"id": "item_potion", "name_key": "item.potion.name", "description_key": "item.potion.desc",
   "buy_price": 30, "sell_price": 10}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG005") != nullptr);
}

TEST_CASE("Validator: duplicate quest ID", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"], "next_quest_ids": []},
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": [], "next_quest_ids": []}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG005") != nullptr);
}

TEST_CASE("Validator: missing NPC reference", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_does_not_exist", "reward_item_ids": ["item_potion"], "next_quest_ids": []}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG006") != nullptr);
}

TEST_CASE("Validator: missing item reference in quest", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_ghost"], "next_quest_ids": []}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG006") != nullptr);
}

TEST_CASE("Validator: missing quest reference in next_quest_ids", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"], "next_quest_ids": ["quest_ghost"]}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG006") != nullptr);
}

TEST_CASE("Validator: duplicate reward_item_ids", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion", "item_potion"], "next_quest_ids": []}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG007") != nullptr);
}

TEST_CASE("Validator: duplicate next_quest_ids", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"],
   "next_quest_ids": ["quest_b", "quest_b"]},
  {"id": "quest_b", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": [], "next_quest_ids": []}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG007") != nullptr);
}

TEST_CASE("Validator: self-referencing quest", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"],
   "next_quest_ids": ["quest_intro"]}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG017") != nullptr);
}

TEST_CASE("Validator: NPC max_hp out of range", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("npcs.json", R"([
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 0, "attack": 5, "defense": 10}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG008") != nullptr);
}

TEST_CASE("Validator: item negative buy_price", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("items.json", R"([
  {"id": "item_potion", "name_key": "item.potion.name", "description_key": "item.potion.desc",
   "buy_price": -5, "sell_price": 20}
])");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG008") != nullptr);
}

TEST_CASE("Validator: sell_price > buy_price is a warning GDG012", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("items.json", R"([
  {"id": "item_potion", "name_key": "item.potion.name", "description_key": "item.potion.desc",
   "buy_price": 10, "sell_price": 100}
])");

    auto r = load_validate(dir);
    REQUIRE_FALSE(r.has_errors());
    REQUIRE(find_diag(r, "GDG012") != nullptr);
    REQUIRE(find_diag(r, "GDG012")->severity == Severity::Warning);
}

TEST_CASE("Validator: missing localization file", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    // manifest requires "fr" locale but no fr.json
    dir.write("manifest.json", R"({
  "schema_version": 1,
  "required_locales": ["en", "fr"],
  "start_quest_ids": ["quest_intro"]
})");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG009") != nullptr);
}

TEST_CASE("Validator: missing localization key", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    // Remove one key from en.json
    dir.write("localization/en.json", R"({
  "npc.guide.name": "Guide",
  "item.potion.name": "Potion",
  "item.potion.desc": "Restores health.",
  "quest.intro.title": "The Beginning"
})");
    // quest.intro.desc is missing

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG010") != nullptr);
}

TEST_CASE("Validator: empty localization value", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("localization/en.json", R"({
  "npc.guide.name": "Guide",
  "item.potion.name": "",
  "item.potion.desc": "Restores health.",
  "quest.intro.title": "The Beginning",
  "quest.intro.desc": "Speak with the guide."
})");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG011") != nullptr);
}

TEST_CASE("Validator: unused localization key is a warning", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("localization/en.json", R"({
  "npc.guide.name": "Guide",
  "item.potion.name": "Potion",
  "item.potion.desc": "Restores health.",
  "quest.intro.title": "The Beginning",
  "quest.intro.desc": "Speak with the guide.",
  "unused.key": "This key is not used."
})");

    auto r = load_validate(dir);
    REQUIRE_FALSE(r.has_errors());
    REQUIRE(find_diag(r, "GDG015") != nullptr);
    REQUIRE(find_diag(r, "GDG015")->severity == Severity::Warning);
}

TEST_CASE("Validator: start quest does not exist", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("manifest.json", R"({
  "schema_version": 1,
  "required_locales": ["en"],
  "start_quest_ids": ["quest_nonexistent"]
})");

    auto r = load_validate(dir);
    REQUIRE(find_diag(r, "GDG016") != nullptr);
}

TEST_CASE("Validator: warnings-as-errors test", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("items.json", R"([
  {"id": "item_potion", "name_key": "item.potion.name", "description_key": "item.potion.desc",
   "buy_price": 10, "sell_price": 100}
])");

    auto r = load_validate(dir);
    // Without warnings-as-errors: should not have errors
    REQUIRE_FALSE(r.has_errors());
    REQUIRE(r.has_warnings());
    // Simulate warnings-as-errors by checking has_warnings
    bool would_fail_wae = r.has_errors() || r.has_warnings();
    REQUIRE(would_fail_wae);
}

TEST_CASE("Validator: diagnostics are sorted deterministically", "[validator]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    // Introduce multiple errors
    dir.write("npcs.json", R"([
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 0, "attack": 5, "defense": 10},
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 100, "attack": 5, "defense": 10}
])");

    auto r = load_validate(dir);
    // All errors should come before warnings
    bool found_warning = false;
    for (const auto& d : r.diagnostics) {
        if (d.severity == Severity::Warning) found_warning = true;
        if (found_warning) REQUIRE(d.severity != Severity::Error);
    }
}

TEST_CASE("Validator: duplicate_id sample", "[validator]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "duplicate_id");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG005") != nullptr);
}

TEST_CASE("Validator: missing_reference sample", "[validator]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "missing_reference");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG006") != nullptr);
}

TEST_CASE("Validator: missing_localization sample", "[validator]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "missing_localization");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG009") != nullptr);
}

TEST_CASE("Validator: invalid_numbers sample", "[validator]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "invalid_numbers");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG008") != nullptr);
}

TEST_CASE("Validator: unused_localization sample produces warning", "[validator]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "unused_localization");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE_FALSE(r.has_errors());
    REQUIRE(find_diag(r, "GDG015") != nullptr);
}
