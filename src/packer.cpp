#include "gamedataguard/packer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

namespace gamedataguard {

using json = nlohmann::json;

namespace {

// Write a uint32_t in little-endian byte order
void write_u32_le(std::ostream& out, uint32_t value) {
    uint8_t bytes[4];
    bytes[0] = static_cast<uint8_t>(value & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    out.write(reinterpret_cast<const char*>(bytes), 4);
}

// Write a uint64_t in little-endian byte order
void write_u64_le(std::ostream& out, uint64_t value) {
    uint8_t bytes[8];
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<uint8_t>(value & 0xFF);
        value >>= 8;
    }
    out.write(reinterpret_cast<const char*>(bytes), 8);
}

// Build the canonical JSON payload
json build_canonical_payload(const GameData& data) {
    // Manifest (sort required_locales and start_quest_ids)
    json manifest_json;
    manifest_json["schema_version"] = data.manifest.schema_version;

    std::vector<std::string> sorted_locales = data.manifest.required_locales;
    std::sort(sorted_locales.begin(), sorted_locales.end());
    manifest_json["required_locales"] = sorted_locales;

    std::vector<std::string> sorted_starts = data.manifest.start_quest_ids;
    std::sort(sorted_starts.begin(), sorted_starts.end());
    manifest_json["start_quest_ids"] = sorted_starts;

    // NPCs sorted by ID
    std::vector<const Npc*> sorted_npcs;
    for (const auto& npc : data.npcs)
        sorted_npcs.push_back(&npc);
    std::sort(sorted_npcs.begin(), sorted_npcs.end(),
        [](const Npc* a, const Npc* b) { return a->id < b->id; });

    json npcs_json = json::array();
    for (const auto* npc : sorted_npcs) {
        json n;
        n["id"]       = npc->id;
        n["name_key"] = npc->name_key;
        n["max_hp"]   = npc->max_hp;
        n["attack"]   = npc->attack;
        n["defense"]  = npc->defense;
        npcs_json.push_back(std::move(n));
    }

    // Items sorted by ID
    std::vector<const Item*> sorted_items;
    for (const auto& item : data.items)
        sorted_items.push_back(&item);
    std::sort(sorted_items.begin(), sorted_items.end(),
        [](const Item* a, const Item* b) { return a->id < b->id; });

    json items_json = json::array();
    for (const auto* item : sorted_items) {
        json i;
        i["id"]              = item->id;
        i["name_key"]        = item->name_key;
        i["description_key"] = item->description_key;
        i["buy_price"]       = item->buy_price;
        i["sell_price"]      = item->sell_price;
        items_json.push_back(std::move(i));
    }

    // Quests sorted by ID, with inner arrays sorted
    std::vector<const Quest*> sorted_quests;
    for (const auto& quest : data.quests)
        sorted_quests.push_back(&quest);
    std::sort(sorted_quests.begin(), sorted_quests.end(),
        [](const Quest* a, const Quest* b) { return a->id < b->id; });

    json quests_json = json::array();
    for (const auto* quest : sorted_quests) {
        json q;
        q["id"]              = quest->id;
        q["title_key"]       = quest->title_key;
        q["description_key"] = quest->description_key;
        q["giver_npc_id"]    = quest->giver_npc_id;

        std::vector<std::string> sorted_rewards = quest->reward_item_ids;
        std::sort(sorted_rewards.begin(), sorted_rewards.end());
        q["reward_item_ids"] = sorted_rewards;

        std::vector<std::string> sorted_next = quest->next_quest_ids;
        std::sort(sorted_next.begin(), sorted_next.end());
        q["next_quest_ids"] = sorted_next;

        quests_json.push_back(std::move(q));
    }

    // Localization: locale names sorted, keys sorted (std::map is already ordered)
    json loc_json;
    for (const auto& [locale, locale_data] : data.localization) {
        json locale_json;
        // std::map iterates in key order, so this is already deterministic
        for (const auto& [key, val] : locale_data) {
            locale_json[key] = val;
        }
        loc_json[locale] = std::move(locale_json);
    }

    json payload;
    payload["manifest"]     = std::move(manifest_json);
    payload["npcs"]         = std::move(npcs_json);
    payload["items"]        = std::move(items_json);
    payload["quests"]       = std::move(quests_json);
    payload["localization"] = std::move(loc_json);

    return payload;
}

} // namespace

std::optional<std::string> write_pack(
    const GameData& data,
    const std::filesystem::path& output_path)
{
    // Build canonical payload
    json payload = build_canonical_payload(data);
    // Use dump with no indent and sorted keys for maximal determinism
    std::string payload_str = payload.dump(-1, ' ', false,
        nlohmann::detail::error_handler_t::strict);

    auto tmp_path = output_path;
    tmp_path += ".tmp";

    {
        std::ofstream out(tmp_path, std::ios::binary);
        if (!out.is_open()) {
            return "Cannot open output file for writing: " + tmp_path.string();
        }

        // Magic: "GDG1"
        out.write("GDG1", 4);

        // Format version: 1
        write_u32_le(out, 1u);

        // Payload size
        write_u64_le(out, static_cast<uint64_t>(payload_str.size()));

        // Payload
        out.write(payload_str.data(), static_cast<std::streamsize>(payload_str.size()));

        if (!out.good()) {
            out.close();
            std::error_code ec;
            std::filesystem::remove(tmp_path, ec);
            return "Error writing pack file: " + tmp_path.string();
        }
    }

    // Atomically rename to final destination
    std::error_code ec;
    std::filesystem::rename(tmp_path, output_path, ec);
    if (ec) {
        std::filesystem::remove(tmp_path, ec);
        return "Failed to finalize pack file: " + ec.message();
    }

    return std::nullopt; // success
}

} // namespace gamedataguard
