#pragma once

#include "data_model.hpp"
#include "validation_result.hpp"
#include <filesystem>
#include <variant>
#include <string>

namespace gamedataguard {

// Represents either a fatal load error (exit code 2) or a successfully loaded dataset
struct LoadError {
    std::string message;
};

struct LoadSuccess {
    GameData data;
};

using LoadResult = std::variant<LoadSuccess, LoadError>;

LoadResult load_game_data(const std::filesystem::path& data_dir);

} // namespace gamedataguard
