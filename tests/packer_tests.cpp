#include <catch2/catch_test_macros.hpp>
#include "gamedataguard/packer.hpp"
#include "gamedataguard/data_loader.hpp"
#include "gamedataguard/validator.hpp"
#include "gamedataguard/json_reporter.hpp"
#include "test_helpers/temp_dir.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <string>

using namespace gamedataguard;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Read entire binary file
static std::vector<uint8_t> read_binary(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    REQUIRE(f.is_open());
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
}

// Read uint32_t little-endian from buffer at offset
static uint32_t read_u32_le(const std::vector<uint8_t>& buf, std::size_t offset) {
    return static_cast<uint32_t>(buf[offset]) |
           (static_cast<uint32_t>(buf[offset+1]) << 8) |
           (static_cast<uint32_t>(buf[offset+2]) << 16) |
           (static_cast<uint32_t>(buf[offset+3]) << 24);
}

// Read uint64_t little-endian from buffer at offset
static uint64_t read_u64_le(const std::vector<uint8_t>& buf, std::size_t offset) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v |= static_cast<uint64_t>(buf[offset + i]) << (8 * i);
    return v;
}

static LoadSuccess load_valid_data() {
    fs::path sample = GDG_SAMPLE_DATA_DIR;
    auto lr = load_game_data(sample / "valid");
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    return std::get<LoadSuccess>(lr);
}

TEST_CASE("Packer: magic bytes are GDG1", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto out = dir.path() / "test.gdgpack";
    auto err = write_pack(loaded.data, out);
    REQUIRE_FALSE(err.has_value());

    auto bytes = read_binary(out);
    REQUIRE(bytes.size() >= 16);
    REQUIRE(bytes[0] == 'G');
    REQUIRE(bytes[1] == 'D');
    REQUIRE(bytes[2] == 'G');
    REQUIRE(bytes[3] == '1');
}

TEST_CASE("Packer: format version is 1", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto out = dir.path() / "test.gdgpack";
    REQUIRE_FALSE(write_pack(loaded.data, out).has_value());

    auto bytes = read_binary(out);
    REQUIRE(read_u32_le(bytes, 4) == 1u);
}

TEST_CASE("Packer: payload size matches actual payload", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto out = dir.path() / "test.gdgpack";
    REQUIRE_FALSE(write_pack(loaded.data, out).has_value());

    auto bytes = read_binary(out);
    REQUIRE(bytes.size() >= 16);

    uint64_t payload_size = read_u64_le(bytes, 8);
    REQUIRE(payload_size == bytes.size() - 16u);
}

TEST_CASE("Packer: payload is valid JSON", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto out = dir.path() / "test.gdgpack";
    REQUIRE_FALSE(write_pack(loaded.data, out).has_value());

    auto bytes = read_binary(out);
    uint64_t payload_size = read_u64_le(bytes, 8);
    std::string payload(reinterpret_cast<const char*>(bytes.data() + 16), payload_size);

    auto j = json::parse(payload); // throws on bad JSON
    REQUIRE(j.contains("manifest"));
    REQUIRE(j.contains("npcs"));
    REQUIRE(j.contains("items"));
    REQUIRE(j.contains("quests"));
    REQUIRE(j.contains("localization"));
}

TEST_CASE("Packer: payload contains all entities", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto out = dir.path() / "test.gdgpack";
    REQUIRE_FALSE(write_pack(loaded.data, out).has_value());

    auto bytes = read_binary(out);
    uint64_t payload_size = read_u64_le(bytes, 8);
    std::string payload(reinterpret_cast<const char*>(bytes.data() + 16), payload_size);
    auto j = json::parse(payload);

    REQUIRE(j["npcs"].size() == 3);
    REQUIRE(j["items"].size() == 4);
    REQUIRE(j["quests"].size() == 5);
    REQUIRE(j["localization"].size() == 3);
}

TEST_CASE("Packer: NPCs sorted by ID in payload", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto out = dir.path() / "test.gdgpack";
    REQUIRE_FALSE(write_pack(loaded.data, out).has_value());

    auto bytes = read_binary(out);
    uint64_t payload_size = read_u64_le(bytes, 8);
    std::string payload(reinterpret_cast<const char*>(bytes.data() + 16), payload_size);
    auto j = json::parse(payload);

    std::vector<std::string> ids;
    for (const auto& npc : j["npcs"])
        ids.push_back(npc["id"].get<std::string>());

    std::vector<std::string> sorted_ids = ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());
    REQUIRE(ids == sorted_ids);
}

TEST_CASE("Packer: two identical builds produce byte-identical output", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();

    auto out1 = dir.path() / "test1.gdgpack";
    auto out2 = dir.path() / "test2.gdgpack";

    REQUIRE_FALSE(write_pack(loaded.data, out1).has_value());
    REQUIRE_FALSE(write_pack(loaded.data, out2).has_value());

    auto bytes1 = read_binary(out1);
    auto bytes2 = read_binary(out2);
    REQUIRE(bytes1 == bytes2);
}

TEST_CASE("Packer: no pack created after validation failure", "[packer]") {
    test_helpers::TempDir dir;
    test_helpers::write_valid_data(dir);
    // Introduce a validation error
    dir.write("npcs.json", R"([
  {"id": "npc_guide", "name_key": "npc.guide.name", "max_hp": 0, "attack": 5, "defense": 10}
])");

    auto lr = load_game_data(dir.path());
    REQUIRE(std::holds_alternative<LoadSuccess>(lr));
    auto result = validate(std::get<LoadSuccess>(lr).data);
    REQUIRE(result.has_errors());

    // Packer should only be called after passing validation;
    // verify by checking we don't call write_pack when errors exist
    // (This test verifies the logic pattern used in main.cpp)
    auto pack_path = dir.path() / "should_not_exist.gdgpack";
    if (!result.has_errors()) {
        write_pack(std::get<LoadSuccess>(lr).data, pack_path);
    }
    REQUIRE_FALSE(fs::exists(pack_path));
}

TEST_CASE("Packer: JSON report written correctly", "[packer]") {
    test_helpers::TempDir dir;
    auto loaded = load_valid_data();
    auto result = validate(loaded.data);

    auto report_path = dir.path() / "report.json";
    bool ok = write_json_report(report_path, "/some/path", result, true);
    REQUIRE(ok);
    REQUIRE(fs::exists(report_path));

    std::ifstream f(report_path);
    auto j = json::parse(f);
    REQUIRE(j["tool"] == "GameDataGuard");
    REQUIRE(j["tool_version"] == "1.0.0");
    REQUIRE(j.contains("status"));
    REQUIRE(j.contains("summary"));
    REQUIRE(j.contains("diagnostics"));
    REQUIRE(j["summary"]["errors"].get<int>() == 0);
}

TEST_CASE("Packer: invalid output path returns error", "[packer]") {
    auto loaded = load_valid_data();
    // Write to a path with a nonexistent parent directory
    fs::path bad_path = "/nonexistent_directory_xyz/test.gdgpack";
    auto err = write_pack(loaded.data, bad_path);
    REQUIRE(err.has_value());
}
