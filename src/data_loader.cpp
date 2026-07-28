#include "gamedataguard/data_loader.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <string>

namespace gamedataguard {

using json = nlohmann::json;

namespace {

// Read entire file as string. Returns empty optional on failure.
std::optional<std::string> read_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Parse JSON from string. Returns empty optional on parse error.
std::optional<json> parse_json(const std::string& text) {
    try {
        return json::parse(text);
    } catch (...) {
        return std::nullopt;
    }
}

// Load mandatory JSON file. Returns LoadError on missing or malformed.
std::optional<json> load_required_json(
    const std::filesystem::path& path,
    const std::string& filename,
    LoadResult& error_out)
{
    auto text = read_file(path);
    if (!text) {
        error_out = LoadError{"Cannot open required file: " + filename};
        return std::nullopt;
    }
    auto j = parse_json(*text);
    if (!j) {
        error_out = LoadError{"Malformed JSON in file: " + filename};
        return std::nullopt;
    }
    return j;
}

LoadResult load_manifest(const std::filesystem::path& dir, Manifest& out) {
    LoadResult err;
    auto path = dir / "manifest.json";
    auto j = load_required_json(path, "manifest.json", err);
    if (!j) return err;

    if (!j->is_object())
        return LoadError{"manifest.json: root must be a JSON object"};

    // schema_version
    if (!j->contains("schema_version") || !(*j)["schema_version"].is_number_integer())
        return LoadError{"manifest.json: missing or invalid 'schema_version'"};
    out.schema_version = (*j)["schema_version"].get<int>();

    // required_locales
    if (!j->contains("required_locales") || !(*j)["required_locales"].is_array())
        return LoadError{"manifest.json: missing or invalid 'required_locales'"};
    for (const auto& loc : (*j)["required_locales"]) {
        if (!loc.is_string())
            return LoadError{"manifest.json: 'required_locales' entries must be strings"};
        out.required_locales.push_back(loc.get<std::string>());
    }

    // start_quest_ids
    if (!j->contains("start_quest_ids") || !(*j)["start_quest_ids"].is_array())
        return LoadError{"manifest.json: missing or invalid 'start_quest_ids'"};
    for (const auto& qid : (*j)["start_quest_ids"]) {
        if (!qid.is_string())
            return LoadError{"manifest.json: 'start_quest_ids' entries must be strings"};
        out.start_quest_ids.push_back(qid.get<std::string>());
    }

    return LoadSuccess{};
}

LoadResult load_npcs(const std::filesystem::path& dir, std::vector<Npc>& out) {
    LoadResult err;
    auto path = dir / "npcs.json";
    auto j = load_required_json(path, "npcs.json", err);
    if (!j) return err;

    if (!j->is_array())
        return LoadError{"npcs.json: root must be a JSON array"};

    int idx = 0;
    for (const auto& item : *j) {
        Npc npc;
        std::string prefix = "npcs.json:/";
        prefix += std::to_string(idx);

        if (!item.is_object())
            return LoadError{prefix + ": each NPC must be a JSON object"};
        if (!item.contains("id") || !item["id"].is_string())
            return LoadError{prefix + ": missing or invalid 'id'"};
        npc.id = item["id"].get<std::string>();

        if (!item.contains("name_key") || !item["name_key"].is_string())
            return LoadError{prefix + ": missing or invalid 'name_key'"};
        npc.name_key = item["name_key"].get<std::string>();

        if (!item.contains("max_hp") || !item["max_hp"].is_number_integer())
            return LoadError{prefix + ": missing or invalid 'max_hp'"};
        npc.max_hp = item["max_hp"].get<int>();

        if (!item.contains("attack") || !item["attack"].is_number_integer())
            return LoadError{prefix + ": missing or invalid 'attack'"};
        npc.attack = item["attack"].get<int>();

        if (!item.contains("defense") || !item["defense"].is_number_integer())
            return LoadError{prefix + ": missing or invalid 'defense'"};
        npc.defense = item["defense"].get<int>();

        out.push_back(std::move(npc));
        ++idx;
    }
    return LoadSuccess{};
}

LoadResult load_items(const std::filesystem::path& dir, std::vector<Item>& out) {
    LoadResult err;
    auto path = dir / "items.json";
    auto j = load_required_json(path, "items.json", err);
    if (!j) return err;

    if (!j->is_array())
        return LoadError{"items.json: root must be a JSON array"};

    int idx = 0;
    for (const auto& elem : *j) {
        Item item;
        std::string prefix = "items.json:/";
        prefix += std::to_string(idx);

        if (!elem.is_object())
            return LoadError{prefix + ": each item must be a JSON object"};
        if (!elem.contains("id") || !elem["id"].is_string())
            return LoadError{prefix + ": missing or invalid 'id'"};
        item.id = elem["id"].get<std::string>();

        if (!elem.contains("name_key") || !elem["name_key"].is_string())
            return LoadError{prefix + ": missing or invalid 'name_key'"};
        item.name_key = elem["name_key"].get<std::string>();

        if (!elem.contains("description_key") || !elem["description_key"].is_string())
            return LoadError{prefix + ": missing or invalid 'description_key'"};
        item.description_key = elem["description_key"].get<std::string>();

        if (!elem.contains("buy_price") || !elem["buy_price"].is_number_integer())
            return LoadError{prefix + ": missing or invalid 'buy_price'"};
        item.buy_price = elem["buy_price"].get<int>();

        if (!elem.contains("sell_price") || !elem["sell_price"].is_number_integer())
            return LoadError{prefix + ": missing or invalid 'sell_price'"};
        item.sell_price = elem["sell_price"].get<int>();

        out.push_back(std::move(item));
        ++idx;
    }
    return LoadSuccess{};
}

LoadResult load_quests(const std::filesystem::path& dir, std::vector<Quest>& out) {
    LoadResult err;
    auto path = dir / "quests.json";
    auto j = load_required_json(path, "quests.json", err);
    if (!j) return err;

    if (!j->is_array())
        return LoadError{"quests.json: root must be a JSON array"};

    int idx = 0;
    for (const auto& elem : *j) {
        Quest quest;
        std::string prefix = "quests.json:/";
        prefix += std::to_string(idx);

        if (!elem.is_object())
            return LoadError{prefix + ": each quest must be a JSON object"};
        if (!elem.contains("id") || !elem["id"].is_string())
            return LoadError{prefix + ": missing or invalid 'id'"};
        quest.id = elem["id"].get<std::string>();

        if (!elem.contains("title_key") || !elem["title_key"].is_string())
            return LoadError{prefix + ": missing or invalid 'title_key'"};
        quest.title_key = elem["title_key"].get<std::string>();

        if (!elem.contains("description_key") || !elem["description_key"].is_string())
            return LoadError{prefix + ": missing or invalid 'description_key'"};
        quest.description_key = elem["description_key"].get<std::string>();

        if (!elem.contains("giver_npc_id") || !elem["giver_npc_id"].is_string())
            return LoadError{prefix + ": missing or invalid 'giver_npc_id'"};
        quest.giver_npc_id = elem["giver_npc_id"].get<std::string>();

        if (elem.contains("reward_item_ids")) {
            if (!elem["reward_item_ids"].is_array())
                return LoadError{prefix + ": 'reward_item_ids' must be an array"};
            for (const auto& rid : elem["reward_item_ids"]) {
                if (!rid.is_string())
                    return LoadError{prefix + ": 'reward_item_ids' entries must be strings"};
                quest.reward_item_ids.push_back(rid.get<std::string>());
            }
        }

        if (elem.contains("next_quest_ids")) {
            if (!elem["next_quest_ids"].is_array())
                return LoadError{prefix + ": 'next_quest_ids' must be an array"};
            for (const auto& nid : elem["next_quest_ids"]) {
                if (!nid.is_string())
                    return LoadError{prefix + ": 'next_quest_ids' entries must be strings"};
                quest.next_quest_ids.push_back(nid.get<std::string>());
            }
        }

        out.push_back(std::move(quest));
        ++idx;
    }
    return LoadSuccess{};
}

LoadResult load_localization(
    const std::filesystem::path& dir,
    const std::vector<std::string>& required_locales,
    LocalizationData& out)
{
    auto loc_dir = dir / "localization";

    for (const auto& locale : required_locales) {
        auto path = loc_dir / (locale + ".json");
        auto text = read_file(path);
        if (!text) {
            // Missing locale file is a validation error (GDG009), not a fatal load error.
            // We skip it here; the validator will catch it.
            continue;
        }
        auto j = parse_json(*text);
        if (!j) {
            return LoadError{"Malformed JSON in localization file: " + locale + ".json"};
        }
        if (!j->is_object()) {
            return LoadError{"localization/" + locale + ".json: root must be a JSON object"};
        }
        LocaleData locale_data;
        for (const auto& [key, val] : j->items()) {
            if (!val.is_string()) {
                return LoadError{
                    "localization/" + locale + ".json: value for key '" + key +
                    "' must be a string"};
            }
            locale_data[key] = val.get<std::string>();
        }
        out[locale] = std::move(locale_data);
    }
    return LoadSuccess{};
}

} // namespace

LoadResult load_game_data(const std::filesystem::path& data_dir) {
    GameData data;

    // Load manifest
    {
        auto result = load_manifest(data_dir, data.manifest);
        if (std::holds_alternative<LoadError>(result))
            return result;
    }

    // Load NPCs
    {
        auto result = load_npcs(data_dir, data.npcs);
        if (std::holds_alternative<LoadError>(result))
            return result;
    }

    // Load items
    {
        auto result = load_items(data_dir, data.items);
        if (std::holds_alternative<LoadError>(result))
            return result;
    }

    // Load quests
    {
        auto result = load_quests(data_dir, data.quests);
        if (std::holds_alternative<LoadError>(result))
            return result;
    }

    // Load localization
    {
        auto result = load_localization(data_dir, data.manifest.required_locales, data.localization);
        if (std::holds_alternative<LoadError>(result))
            return result;
    }

    return LoadSuccess{std::move(data)};
}

} // namespace gamedataguard
