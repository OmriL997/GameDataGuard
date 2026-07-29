#include <catch2/catch_test_macros.hpp>
#include "gamedataguard/quest_graph.hpp"
#include "gamedataguard/data_loader.hpp"
#include "gamedataguard/validator.hpp"
#include "gamedataguard/diagnostic.hpp"
#include "test_helpers/temp_dir.hpp"
#include <filesystem>

using namespace gamedataguard;
namespace fs = std::filesystem;

static const Diagnostic* find_diag(const ValidationResult& r, const std::string& code) {
    for (const auto& d : r.diagnostics)
        if (d.code == code) return &d;
    return nullptr;
}

TEST_CASE("QuestGraph: no quests -> no diagnostics", "[quest_graph]") {
    ValidationResult result;
    validate_quest_graph({}, {}, result);
    REQUIRE(result.diagnostics.empty());
}

TEST_CASE("QuestGraph: self-reference detected", "[quest_graph]") {
    std::vector<Quest> quests = {
        {"quest_a", "t", "d", "npc_x", {}, {"quest_a"}}
    };
    ValidationResult result;
    // Self-reference is handled in validator.cpp, not quest_graph.cpp,
    // but let's confirm GDG017 via the full validator
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"],
   "next_quest_ids": ["quest_intro"]}
])");

    auto lr = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG017") != nullptr);
}

TEST_CASE("QuestGraph: cycle detected", "[quest_graph]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "quest_cycle");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG013") != nullptr);
    // Cycle message should reference quest IDs
    const auto* d = find_diag(r, "GDG013");
    REQUIRE(d->message.find("quest") != std::string::npos);
}

TEST_CASE("QuestGraph: unreachable quest detected", "[quest_graph]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "invalid" / "unreachable_quest");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG014") != nullptr);
    const auto* d = find_diag(r, "GDG014");
    REQUIRE(d->message.find("quest_orphan") != std::string::npos);
}

TEST_CASE("QuestGraph: valid branching quest graph has no cycle/unreachable", "[quest_graph]") {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "valid");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG013") == nullptr);
    REQUIRE(find_diag(r, "GDG014") == nullptr);
}

TEST_CASE("QuestGraph: two-node cycle detected", "[quest_graph]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"], "next_quest_ids": ["quest_b"]},
  {"id": "quest_b", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": [], "next_quest_ids": ["quest_intro"]}
])");
    dir.write("localization/en.json", R"({
  "npc.guide.name": "Guide",
  "item.potion.name": "Potion",
  "item.potion.desc": "Restores health.",
  "quest.intro.title": "The Beginning",
  "quest.intro.desc": "Speak with the guide."
})");

    auto lr = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(find_diag(r, "GDG013") != nullptr);
}

TEST_CASE("QuestGraph: multiple unreachable quests all reported", "[quest_graph]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"], "next_quest_ids": []},
  {"id": "quest_orphan1", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": [], "next_quest_ids": []},
  {"id": "quest_orphan2", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": [], "next_quest_ids": []}
])");
    dir.write("localization/en.json", R"({
  "npc.guide.name": "Guide",
  "item.potion.name": "Potion",
  "item.potion.desc": "Restores health.",
  "quest.intro.title": "The Beginning",
  "quest.intro.desc": "Speak with the guide."
})");

    auto lr = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto r = validate(std::get<LoadSuccess>(lr).data);

    int unreachable = 0;
    for (const auto& d : r.diagnostics)
        if (d.code == "GDG014") ++unreachable;
    REQUIRE(unreachable == 2);
}
