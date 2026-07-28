#pragma once

#include <string>
#include <vector>
#include <map>

namespace gamedataguard {

struct Npc {
    std::string id;
    std::string name_key;
    int max_hp{0};
    int attack{0};
    int defense{0};
};

struct Item {
    std::string id;
    std::string name_key;
    std::string description_key;
    int buy_price{0};
    int sell_price{0};
};

struct Quest {
    std::string id;
    std::string title_key;
    std::string description_key;
    std::string giver_npc_id;
    std::vector<std::string> reward_item_ids;
    std::vector<std::string> next_quest_ids;
};

struct Manifest {
    int schema_version{0};
    std::vector<std::string> required_locales;
    std::vector<std::string> start_quest_ids;
};

// Localization: locale -> (key -> value)
using LocaleData = std::map<std::string, std::string>;
using LocalizationData = std::map<std::string, LocaleData>;

struct GameData {
    Manifest manifest;
    std::vector<Npc> npcs;
    std::vector<Item> items;
    std::vector<Quest> quests;
    LocalizationData localization;
};

} // namespace gamedataguard
