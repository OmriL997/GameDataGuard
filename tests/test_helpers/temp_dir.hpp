#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>

namespace test_helpers {

// RAII temporary directory that is cleaned up when the object goes out of scope.
class TempDir {
public:
    TempDir() {
        namespace fs = std::filesystem;
        auto base = fs::temp_directory_path();
        // Create a unique directory name using a counter
        static int counter = 0;
        ++counter;
        path_ = base / ("gdg_test_" + std::to_string(counter) + "_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    TempDir(TempDir&&) = delete;
    TempDir& operator=(TempDir&&) = delete;

    const std::filesystem::path& path() const { return path_; }

    // Write a file relative to this temp directory
    void write(const std::string& relative_path, const std::string& content) const {
        auto full_path = path_ / relative_path;
        std::filesystem::create_directories(full_path.parent_path());
        std::ofstream f(full_path, std::ios::binary);
        if (!f.is_open())
            throw std::runtime_error("Cannot write test file: " + full_path.string());
        f << content;
    }

private:
    std::filesystem::path path_;
};

// Write valid minimal data to a TempDir.
// Caller can overwrite individual files as needed.
inline void write_valid_data(const TempDir& dir) {
    dir.write("manifest.json", R"({
  "schema_version": 1,
  "required_locales": ["en"],
  "start_quest_ids": ["quest_intro"]
})");

    dir.write("npcs.json", R"([
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 100, "attack": 5, "defense": 10}
])");

    dir.write("items.json", R"([
  {"id": "item_potion", "name_key": "item.potion.name", "description_key": "item.potion.desc", "buy_price": 50, "sell_price": 20}
])");

    dir.write("quests.json", R"([
  {"id": "quest_intro", "title_key": "quest.intro.title", "description_key": "quest.intro.desc",
   "giver_npc_id": "npc_guide", "reward_item_ids": ["item_potion"], "next_quest_ids": []}
])");

    dir.write("localization/en.json", R"({
  "npc.guide.name": "Guide",
  "item.potion.name": "Potion",
  "item.potion.desc": "Restores health.",
  "quest.intro.title": "The Beginning",
  "quest.intro.desc": "Speak with the guide."
})");
}

} // namespace test_helpers
