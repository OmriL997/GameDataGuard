#include "gamedataguard/quest_graph.hpp"
#include "gamedataguard/diagnostic.hpp"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>

namespace gamedataguard {

namespace {

enum class NodeColor { White, Gray, Black };

struct GraphState {
    std::map<std::string, std::vector<std::string>> adj; // sorted edges for determinism
    std::map<std::string, NodeColor> color;
    std::vector<std::string> dfs_stack; // tracks current DFS path
    bool cycle_found{false};
};

void add_diag(ValidationResult& result, Severity sev, const std::string& code,
              std::optional<std::string> path, std::string msg)
{
    result.diagnostics.push_back({sev, code, "quests.json", std::move(path), std::move(msg)});
}

// DFS for cycle detection.
// Returns true if a cycle was found starting from 'node'.
bool dfs_cycle(GraphState& state, const std::string& node, ValidationResult& result) {
    state.color[node] = NodeColor::Gray;
    state.dfs_stack.push_back(node);

    // Use sorted adjacency for deterministic output
    auto it = state.adj.find(node);
    if (it != state.adj.end()) {
        for (const auto& next : it->second) {
            if (state.color.find(next) == state.color.end() ||
                state.color.at(next) == NodeColor::White)
            {
                if (dfs_cycle(state, next, result)) {
                    state.color[node] = NodeColor::Black;
                    state.dfs_stack.pop_back();
                    return true;
                }
            } else if (state.color.at(next) == NodeColor::Gray) {
                // Found a back edge -> cycle
                // Build cycle path string
                std::string path_str;
                auto start = std::find(state.dfs_stack.begin(), state.dfs_stack.end(), next);
                if (start != state.dfs_stack.end()) {
                    for (auto sit = start; sit != state.dfs_stack.end(); ++sit) {
                        if (!path_str.empty()) path_str += " -> ";
                        path_str += *sit;
                    }
                    path_str += " -> " + next;
                } else {
                    path_str = node + " -> " + next;
                }
                add_diag(result, Severity::Error, "GDG013", std::nullopt,
                    "Quest graph cycle detected: " + path_str);
                state.cycle_found = true;
                state.color[node] = NodeColor::Black;
                state.dfs_stack.pop_back();
                return true;
            }
        }
    }

    state.color[node] = NodeColor::Black;
    state.dfs_stack.pop_back();
    return false;
}

} // namespace

void validate_quest_graph(
    const std::vector<Quest>& quests,
    const std::vector<std::string>& start_quest_ids,
    ValidationResult& result)
{
    if (quests.empty()) return;

    // Build adjacency list (only valid quest IDs as nodes).
    // Use std::map for deterministic iteration.
    std::set<std::string> all_ids;
    for (const auto& q : quests)
        if (!q.id.empty()) all_ids.insert(q.id);

    GraphState state;
    for (const auto& q : quests) {
        if (q.id.empty()) continue;
        std::vector<std::string> neighbors;
        for (const auto& nid : q.next_quest_ids) {
            // Only include edges to existing quests (missing refs already reported)
            if (all_ids.count(nid) && nid != q.id) {
                neighbors.push_back(nid);
            }
        }
        std::sort(neighbors.begin(), neighbors.end());
        state.adj[q.id] = std::move(neighbors);
    }

    // Initialize all nodes as White
    for (const auto& id : all_ids)
        state.color[id] = NodeColor::White;

    // Run DFS from each unvisited node (sorted for determinism)
    for (const auto& id : all_ids) {
        if (state.color[id] == NodeColor::White) {
            dfs_cycle(state, id, result);
        }
    }

    // Reachability analysis from start quest IDs (BFS/DFS)
    std::set<std::string> reachable;
    // Use sorted start IDs for determinism
    std::vector<std::string> sorted_starts(start_quest_ids.begin(), start_quest_ids.end());
    std::sort(sorted_starts.begin(), sorted_starts.end());

    std::vector<std::string> work_queue;
    for (const auto& sid : sorted_starts) {
        if (all_ids.count(sid) && !reachable.count(sid)) {
            reachable.insert(sid);
            work_queue.push_back(sid);
        }
    }

    while (!work_queue.empty()) {
        std::string current = work_queue.back();
        work_queue.pop_back();
        auto it = state.adj.find(current);
        if (it != state.adj.end()) {
            for (const auto& next : it->second) {
                if (!reachable.count(next)) {
                    reachable.insert(next);
                    work_queue.push_back(next);
                }
            }
        }
    }

    // Report unreachable quests (deterministic order)
    for (const auto& id : all_ids) {
        if (!reachable.count(id)) {
            add_diag(result, Severity::Error, "GDG014", std::nullopt,
                "Quest \"" + id + "\" is unreachable from any start quest.");
        }
    }
}

} // namespace gamedataguard
