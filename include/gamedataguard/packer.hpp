#pragma once

#include "data_model.hpp"
#include <filesystem>
#include <string>
#include <optional>

namespace gamedataguard {

// Pack binary format:
//   [0..3]  ASCII magic: "GDG1"
//   [4..7]  uint32_t format version (1), little-endian
//   [8..15] uint64_t payload size in bytes, little-endian
//   [16..]  UTF-8 JSON payload
//
// Returns error message on failure, empty optional on success.
std::optional<std::string> write_pack(
    const GameData& data,
    const std::filesystem::path& output_path);

} // namespace gamedataguard
