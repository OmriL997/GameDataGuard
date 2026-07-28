#include "gamedataguard/validator.hpp"
#include "gamedataguard/quest_graph.hpp"
#include "gamedataguard/diagnostic.hpp"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <string>

namespace gamedataguard {

namespace {

void add(ValidationResult& result, Severity sev, const std::string& code,
         const std::string& file, std::optional<std::string> path, std::string msg)
{
    result.diagnostics.push_back({sev, code, file, std::move(path), std::move(msg)});
}

// ---- Manifest validation ----
void validate_manifest(const Manifest& manifest, ValidationResult& result) {
    const std::string file = "manifest.json";

    if (manifest.schema_version != 1) {
        add(result, Severity::Error, "GDG002", file, "/schema_version",
            "Unsupported schema version: " + std::to_string(manifest.schema_version) +
            ". Expected 1.");
    }

    if (manifest.required_locales.empty()) {
        add(result, Severity::Error, "GDG004", file, "/required_locales",
            "'required_locales' must contain at least one locale.");
    } else {
        // Check for empty locale strings
        int idx = 0;
        for (const auto& loc : manifest.required_locales) {
            if (loc.empty()) {
                add(result, Severity::Error, "GDG004", file,
                    "/required_locales/" + std::to_string(idx),
                    "Locale identifier must not be empty.");
            }
            ++idx;
        }
        // Check uniqueness
        std::unordered_map<std::string, int> seen;
        int i = 0;
        for (const auto& loc : manifest.required_locales) {
            if (!loc.empty()) {
                auto [it, inserted] = seen.emplace(loc, i);
                if (!inserted) {
                    add(result, Severity::Error, "GDG005", file,
                        "/required_locales/" + std::to_string(i),
                        "Duplicate locale identifier: \"" + loc + "\".");
                }
            }
            ++i;
        }
    }

    if (manifest.start_quest_ids.empty()) {
        add(result, Severity::Error, "GDG004", file, "/start_quest_ids",
            "'start_quest_ids' must contain at least one quest ID.");
    }
}

// ---- NPC validation ----
void validate_npcs(const std::vector<Npc>& npcs, ValidationResult& result) {
    const std::string file = "npcs.json";
    std::unordered_map<std::string, int> seen_ids;

    int idx = 0;
    for (const auto& npc : npcs) {
        std::string prefix = "/" + std::to_string(idx);

        if (npc.id.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/id",
                "NPC id must not be empty.");
        } else {
            auto [it, inserted] = seen_ids.emplace(npc.id, idx);
            if (!inserted) {
                add(result, Severity::Error, "GDG005", file, prefix + "/id",
                    "Duplicate NPC id: \"" + npc.id + "\".");
            }
        }

        if (npc.name_key.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/name_key",
                "NPC name_key must not be empty (npc: \"" + npc.id + "\").");
        }

        if (npc.max_hp < 1 || npc.max_hp > 999999) {
            add(result, Severity::Error, "GDG008", file, prefix + "/max_hp",
                "NPC \"" + npc.id + "\" max_hp (" + std::to_string(npc.max_hp) +
                ") must be between 1 and 999999.");
        }

        if (npc.attack < 0 || npc.attack > 999999) {
            add(result, Severity::Error, "GDG008", file, prefix + "/attack",
                "NPC \"" + npc.id + "\" attack (" + std::to_string(npc.attack) +
                ") must be between 0 and 999999.");
        }

        if (npc.defense < 0 || npc.defense > 999999) {
            add(result, Severity::Error, "GDG008", file, prefix + "/defense",
                "NPC \"" + npc.id + "\" defense (" + std::to_string(npc.defense) +
                ") must be between 0 and 999999.");
        }

        ++idx;
    }
}

// ---- Item validation ----
void validate_items(const std::vector<Item>& items, ValidationResult& result) {
    const std::string file = "items.json";
    std::unordered_map<std::string, int> seen_ids;

    int idx = 0;
    for (const auto& item : items) {
        std::string prefix = "/" + std::to_string(idx);

        if (item.id.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/id",
                "Item id must not be empty.");
        } else {
            auto [it, inserted] = seen_ids.emplace(item.id, idx);
            if (!inserted) {
                add(result, Severity::Error, "GDG005", file, prefix + "/id",
                    "Duplicate item id: \"" + item.id + "\".");
            }
        }

        if (item.name_key.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/name_key",
                "Item name_key must not be empty (item: \"" + item.id + "\").");
        }

        if (item.description_key.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/description_key",
                "Item description_key must not be empty (item: \"" + item.id + "\").");
        }

        if (item.buy_price < 0) {
            add(result, Severity::Error, "GDG008", file, prefix + "/buy_price",
                "Item \"" + item.id + "\" buy_price (" + std::to_string(item.buy_price) +
                ") must be zero or greater.");
        }

        if (item.sell_price < 0) {
            add(result, Severity::Error, "GDG008", file, prefix + "/sell_price",
                "Item \"" + item.id + "\" sell_price (" + std::to_string(item.sell_price) +
                ") must be zero or greater.");
        }

        if (item.buy_price >= 0 && item.sell_price > item.buy_price) {
            add(result, Severity::Warning, "GDG012", file, prefix + "/sell_price",
                "Item \"" + item.id + "\" has a sell price (" +
                std::to_string(item.sell_price) + ") greater than its buy price (" +
                std::to_string(item.buy_price) + ").");
        }

        ++idx;
    }
}

// ---- Quest individual-field validation ----
void validate_quests_fields(
    const std::vector<Quest>& quests,
    const std::unordered_set<std::string>& npc_ids,
    const std::unordered_set<std::string>& item_ids,
    const std::unordered_set<std::string>& quest_ids,
    ValidationResult& result)
{
    const std::string file = "quests.json";
    std::unordered_map<std::string, int> seen_ids;

    int idx = 0;
    for (const auto& quest : quests) {
        std::string prefix = "/" + std::to_string(idx);

        if (quest.id.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/id",
                "Quest id must not be empty.");
        } else {
            auto [it, inserted] = seen_ids.emplace(quest.id, idx);
            if (!inserted) {
                add(result, Severity::Error, "GDG005", file, prefix + "/id",
                    "Duplicate quest id: \"" + quest.id + "\".");
            }
        }

        if (quest.title_key.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/title_key",
                "Quest title_key must not be empty (quest: \"" + quest.id + "\").");
        }

        if (quest.description_key.empty()) {
            add(result, Severity::Error, "GDG004", file, prefix + "/description_key",
                "Quest description_key must not be empty (quest: \"" + quest.id + "\").");
        }

        if (!quest.giver_npc_id.empty() && npc_ids.find(quest.giver_npc_id) == npc_ids.end()) {
            add(result, Severity::Error, "GDG006", file, prefix + "/giver_npc_id",
                "Referenced NPC \"" + quest.giver_npc_id + "\" does not exist.");
        }

        // reward_item_ids
        {
            std::unordered_set<std::string> seen_rewards;
            int ridx = 0;
            for (const auto& rid : quest.reward_item_ids) {
                std::string rpath = prefix + "/reward_item_ids/" + std::to_string(ridx);
                if (item_ids.find(rid) == item_ids.end()) {
                    add(result, Severity::Error, "GDG006", file, rpath,
                        "Quest \"" + quest.id + "\": referenced item \"" + rid + "\" does not exist.");
                }
                if (!seen_rewards.insert(rid).second) {
                    add(result, Severity::Error, "GDG007", file, rpath,
                        "Quest \"" + quest.id + "\": duplicate reward_item_id \"" + rid + "\".");
                }
                ++ridx;
            }
        }

        // next_quest_ids
        {
            std::unordered_set<std::string> seen_next;
            int nidx = 0;
            for (const auto& nid : quest.next_quest_ids) {
                std::string npath = prefix + "/next_quest_ids/" + std::to_string(nidx);
                if (nid == quest.id) {
                    add(result, Severity::Error, "GDG017", file, npath,
                        "Quest \"" + quest.id + "\" references itself in next_quest_ids.");
                } else if (quest_ids.find(nid) == quest_ids.end()) {
                    add(result, Severity::Error, "GDG006", file, npath,
                        "Quest \"" + quest.id + "\": referenced next quest \"" + nid +
                        "\" does not exist.");
                }
                if (!seen_next.insert(nid).second) {
                    add(result, Severity::Error, "GDG007", file, npath,
                        "Quest \"" + quest.id + "\": duplicate next_quest_id \"" + nid + "\".");
                }
                ++nidx;
            }
        }

        ++idx;
    }
}

// ---- Manifest start quest validation ----
void validate_manifest_start_quests(
    const Manifest& manifest,
    const std::unordered_set<std::string>& quest_ids,
    ValidationResult& result)
{
    const std::string file = "manifest.json";
    int idx = 0;
    for (const auto& qid : manifest.start_quest_ids) {
        if (qid.empty()) {
            add(result, Severity::Error, "GDG004", file,
                "/start_quest_ids/" + std::to_string(idx),
                "start_quest_ids entry must not be empty.");
        } else if (quest_ids.find(qid) == quest_ids.end()) {
            add(result, Severity::Error, "GDG016", file,
                "/start_quest_ids/" + std::to_string(idx),
                "Start quest \"" + qid + "\" does not exist.");
        }
        ++idx;
    }
}

// ---- Localization validation ----
void validate_localization(
    const GameData& data,
    ValidationResult& result)
{
    const auto& manifest = data.manifest;
    const auto& localization = data.localization;

    // Gather all referenced keys
    std::unordered_set<std::string> referenced_keys;
    for (const auto& npc : data.npcs) {
        if (!npc.name_key.empty()) referenced_keys.insert(npc.name_key);
    }
    for (const auto& item : data.items) {
        if (!item.name_key.empty())        referenced_keys.insert(item.name_key);
        if (!item.description_key.empty()) referenced_keys.insert(item.description_key);
    }
    for (const auto& quest : data.quests) {
        if (!quest.title_key.empty())       referenced_keys.insert(quest.title_key);
        if (!quest.description_key.empty()) referenced_keys.insert(quest.description_key);
    }

    for (const auto& locale : manifest.required_locales) {
        if (locale.empty()) continue;

        std::string loc_file = "localization/" + locale + ".json";
        auto it = localization.find(locale);
        if (it == localization.end()) {
            // File was missing (not loaded)
            add(result, Severity::Error, "GDG009", loc_file, std::nullopt,
                "Missing required localization file for locale \"" + locale + "\".");
            continue;
        }

        const auto& locale_data = it->second;

        // Check all referenced keys exist
        for (const auto& key : referenced_keys) {
            auto kit = locale_data.find(key);
            if (kit == locale_data.end()) {
                add(result, Severity::Error, "GDG010", loc_file, "/" + key,
                    "Missing localization key \"" + key + "\" in locale \"" + locale + "\".");
            } else {
                // Check not empty/whitespace
                const auto& val = kit->second;
                bool whitespace_only = !val.empty() &&
                    val.find_first_not_of(" \t\r\n") == std::string::npos;
                if (val.empty() || whitespace_only) {
                    add(result, Severity::Error, "GDG011", loc_file, "/" + key,
                        "Localization key \"" + key + "\" in locale \"" + locale +
                        "\" has an empty or whitespace-only value.");
                }
            }
        }

        // Check for unused keys (warnings)
        for (const auto& [key, val] : locale_data) {
            if (referenced_keys.find(key) == referenced_keys.end()) {
                add(result, Severity::Warning, "GDG015", loc_file, "/" + key,
                    "Unused localization key \"" + key + "\" in locale \"" + locale + "\".");
            }
        }
    }
}

} // namespace

ValidationResult validate(const GameData& data) {
    ValidationResult result;

    // Build ID sets for cross-reference validation
    std::unordered_set<std::string> npc_ids;
    for (const auto& npc : data.npcs)
        if (!npc.id.empty()) npc_ids.insert(npc.id);

    std::unordered_set<std::string> item_ids;
    for (const auto& item : data.items)
        if (!item.id.empty()) item_ids.insert(item.id);

    std::unordered_set<std::string> quest_ids;
    for (const auto& quest : data.quests)
        if (!quest.id.empty()) quest_ids.insert(quest.id);

    validate_manifest(data.manifest, result);
    validate_npcs(data.npcs, result);
    validate_items(data.items, result);
    validate_quests_fields(data.quests, npc_ids, item_ids, quest_ids, result);
    validate_manifest_start_quests(data.manifest, quest_ids, result);
    validate_quest_graph(data.quests, data.manifest.start_quest_ids, result);
    validate_localization(data, result);

    result.sort();
    return result;
}

} // namespace gamedataguard
