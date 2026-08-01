# GameDataGuard — Validation Reference

Complete rule-by-rule breakdown and diagnostic code table. See the [README](../README.md) for an overview.

---

## Validation Rules

### Manifest (`manifest.json`)

| Field | Rule | Code |
|---|---|---|
| `schema_version` | Must equal `1` | GDG002 |
| `required_locales` | Must contain at least one entry | GDG004 |
| `required_locales[i]` | Each entry must be a non-empty string | GDG004 |
| `required_locales[i]` | No duplicate locale identifiers | GDG005 |
| `start_quest_ids` | Must contain at least one entry | GDG004 |
| `start_quest_ids[i]` | Each entry must be a non-empty string | GDG004 |
| `start_quest_ids[i]` | Each referenced quest must exist in `quests.json` | GDG016 |

### NPCs (`npcs.json`)

| Field | Rule | Code |
|---|---|---|
| `id` | Must be a non-empty string | GDG004 |
| `id` | Must be unique across all NPCs | GDG005 |
| `name_key` | Must be a non-empty string | GDG004 |
| `max_hp` | Must be an integer in the range [1, 999999] | GDG008 |
| `attack` | Must be an integer in the range [0, 999999] | GDG008 |
| `defense` | Must be an integer in the range [0, 999999] | GDG008 |

### Items (`items.json`)

| Field | Rule | Code |
|---|---|---|
| `id` | Must be a non-empty string | GDG004 |
| `id` | Must be unique across all items | GDG005 |
| `name_key` | Must be a non-empty string | GDG004 |
| `description_key` | Must be a non-empty string | GDG004 |
| `buy_price` | Must be ≥ 0 | GDG008 |
| `sell_price` | Must be ≥ 0 | GDG008 |
| `sell_price` | Sell price > buy price raises a warning | GDG012 |

### Quests (`quests.json`)

| Field | Rule | Code |
|---|---|---|
| `id` | Must be a non-empty string | GDG004 |
| `id` | Must be unique across all quests | GDG005 |
| `title_key` | Must be a non-empty string | GDG004 |
| `description_key` | Must be a non-empty string | GDG004 |
| `giver_npc_id` | If present, must reference an existing NPC `id` | GDG006 |
| `reward_item_ids[i]` | Each entry must reference an existing item `id` | GDG006 |
| `reward_item_ids[i]` | No duplicate entries within the same array | GDG007 |
| `next_quest_ids[i]` | Each entry must reference an existing quest `id` | GDG006 |
| `next_quest_ids[i]` | A quest must not reference itself | GDG017 |
| `next_quest_ids[i]` | No duplicate entries within the same array | GDG007 |

### Quest graph (cross-quest analysis)

| Check | Rule | Code |
|---|---|---|
| Cycle detection | The `next_quest_ids` graph must be acyclic; cycles are reported with the full path | GDG013 |
| Reachability | Every quest must be reachable from at least one `start_quest_ids` entry | GDG014 |

### Localization (`localization/<locale>.json`)

| Check | Rule | Code |
|---|---|---|
| File presence | A required locale file must exist on disk | GDG009 |
| Key presence | Every key referenced by NPCs, items, and quests must exist in every required locale | GDG010 |
| Value content | A localization value must not be empty or whitespace-only | GDG011 |
| Unused keys | Keys present in a locale file but not referenced by any entity raise a warning | GDG015 |

Referenced keys are: every `name_key` of NPCs, every `name_key` and `description_key` of items, every `title_key` and `description_key` of quests.

---

## Diagnostic Codes

| Code | Severity | Emitted by | Description |
|---|---|---|---|
| GDG001 | — | — | Reserved |
| GDG002 | Error | `validate_manifest` | `schema_version` is not `1` |
| GDG003 | — | — | Reserved |
| GDG004 | Error | All validators | Required field is empty or missing (non-empty string or non-empty array expected) |
| GDG005 | Error | All validators | Duplicate identifier — two entities share the same `id`, or a list contains a duplicate locale identifier |
| GDG006 | Error | `validate_quests_fields` | Referenced ID does not exist: `giver_npc_id`, `reward_item_ids[i]`, or `next_quest_ids[i]` points to an unknown entity |
| GDG007 | Error | `validate_quests_fields` | Duplicate entry within a single array: `reward_item_ids` or `next_quest_ids` contains the same value more than once |
| GDG008 | Error | `validate_npcs`, `validate_items` | Numeric value outside the allowed range (`max_hp` 1–999999; `attack`/`defense` 0–999999; `buy_price`/`sell_price` ≥ 0) |
| GDG009 | Error | `validate_localization` | A locale listed in `manifest.required_locales` has no corresponding file on disk |
| GDG010 | Error | `validate_localization` | A key referenced by an NPC, item, or quest is absent from a required locale file |
| GDG011 | Error | `validate_localization` | A localization value is empty or contains only whitespace characters |
| GDG012 | Warning | `validate_items` | `sell_price` exceeds `buy_price`; economically suspicious but not necessarily wrong |
| GDG013 | Error | `validate_quest_graph` | A cycle was detected in the quest graph; the diagnostic message includes the full cycle path |
| GDG014 | Error | `validate_quest_graph` | A quest is not reachable from any entry in `manifest.start_quest_ids` |
| GDG015 | Warning | `validate_localization` | A key is present in a locale file but is not referenced by any NPC, item, or quest |
| GDG016 | Error | `validate_manifest_start_quests` | A quest ID listed in `manifest.start_quest_ids` does not exist in `quests.json` |
| GDG017 | Error | `validate_quests_fields` | A quest lists its own `id` in `next_quest_ids` (self-reference) |

### Notes

- **Error** diagnostics cause exit code 1 from the `validate` command and prevent the pack from being written by the `build` command.
- **Warning** diagnostics cause exit code 0 unless `--warnings-as-errors` is supplied, in which case they are treated as errors.
- Codes GDG001 and GDG003 are reserved and not currently emitted.
- All codes are stable identifiers intended for use in CI scripts, suppression lists, and tooling integrations. A code will not be renumbered or repurposed.

---

## Sorting Order of Diagnostics

Diagnostics are sorted before printing and before writing the JSON report. The sort key is:

1. Severity (Error before Warning before Info)
2. File path (lexicographic)
3. JSON pointer path (lexicographic; `nullopt` sorts before any non-empty value)
4. Diagnostic code (lexicographic)

This ordering is stable and deterministic for identical input, ensuring reproducible output in CI.

---

## Exit Codes

| Code | Condition |
|---|---|
| 0 | Validation passed (no errors, no warnings) — or build succeeded |
| 1 | Validation errors found — or warnings with `--warnings-as-errors` |
| 2 | Fatal input failure: file not found, malformed JSON, or internal error |

Load failures (missing required file, malformed JSON) return exit code 2 and are represented as `LoadError`, not as `Diagnostic` objects. They prevent validation from running at all.
