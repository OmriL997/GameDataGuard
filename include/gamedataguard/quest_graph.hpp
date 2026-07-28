#pragma once

#include "data_model.hpp"
#include "validation_result.hpp"
#include <string>
#include <vector>

namespace gamedataguard {

// Validates quest graph properties:
// - self-references
// - cycle detection (DFS)
// - reachability from start quests
// Appends diagnostics to result.
void validate_quest_graph(
    const std::vector<Quest>& quests,
    const std::vector<std::string>& start_quest_ids,
    ValidationResult& result);

} // namespace gamedataguard
