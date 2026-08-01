# GameDataGuard

**GameDataGuard** is a C++ tool that validates, diagnoses, and packages structured game data. It detects broken references, duplicate identifiers, localization gaps, invalid parameters, quest-graph cycles, and unreachable content before the data reaches the runtime build.

> **Scope:** Portfolio-scale internal game-data validation tool. Production-inspired design intended to demonstrate modern C++17 practices across file I/O, data validation, graph analysis, deterministic packaging, and automated testing.

---

## Problem Being Solved

Game studios accumulate structured data files (NPCs, items, quests, localization) that are edited by designers and writers. Before these files reach the runtime build, common problems must be caught:

- Broken cross-references (a quest references an NPC that was renamed)
- Duplicate IDs that cause undefined behavior at runtime
- Missing or empty localization strings
- Quest graphs with cycles or dead-end unreachable quests
- Numeric values outside valid ranges
- Suspicious economic values (sell price > buy price)

GameDataGuard catches all of these at CI time, with stable diagnostic codes, structured JSON reports, and deterministic binary asset packaging.

---

## Main Features

- **Structured validation** with stable diagnostic codes (GDG001–GDG017)
- **Quest graph analysis**: self-reference, cycle detection (DFS), and reachability from start quests
- **Localization completeness checking** across multiple locale files
- **JSON diagnostic reports** written to a specified path
- **Deterministic binary asset packaging** (`.gdgpack` format)
- **Clean exit codes**: 0 = success, 1 = validation failure, 2 = tool/input failure
- **`--warnings-as-errors` mode** for strict CI enforcement
- **No runtime dependencies** beyond the C++ standard library

---

## Terminal Output Example

```
$ gamedataguard validate sample_data/invalid/missing_reference

ERROR GDG006 [quests.json:/0/giver_npc_id]
Referenced NPC "npc_does_not_exist" does not exist.

Validation failed: 1 error, 0 warnings.

$ echo $?
1

$ gamedataguard validate sample_data/invalid/quest_cycle

ERROR GDG013 [quests.json]
Quest graph cycle detected: quest_final -> quest_intro -> quest_forest -> quest_final

Validation failed: 1 error, 0 warnings.

$ gamedataguard build sample_data/valid output.gdgpack --report report.json

Validation passed: no errors, no warnings.
Pack written: output.gdgpack

$ gamedataguard validate sample_data/invalid/unused_localization --warnings-as-errors

WARNING GDG015 [localization/en.json:/unused.key]
Unused localization key "unused.key" in locale "en".

Validation passed with 1 warning.

$ echo $?
1
```

---

## Architecture Overview

```
CLI (cli.cpp / main.cpp)
    └── DataLoader (data_loader.cpp)       reads + parses JSON files
    └── Validator (validator.cpp)          collects ValidationResult diagnostics
         └── QuestGraph (quest_graph.cpp)  DFS cycle + reachability analysis
    └── JsonReporter (json_reporter.cpp)   writes structured JSON report
    └── Packer (packer.cpp)               writes deterministic binary pack
```

Components are separated by responsibility. The validator returns structured results; it does not print. Printing belongs to the CLI layer. The packer only receives data that has passed validation.

See [docs/architecture.md](docs/architecture.md) for the full component and data-flow description.

---

## Repository Structure

```
GameDataGuard/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
├── .editorconfig
├── .clang-format
├── Jenkinsfile
├── include/gamedataguard/
│   ├── cli.hpp
│   ├── data_loader.hpp
│   ├── data_model.hpp
│   ├── diagnostic.hpp
│   ├── json_reporter.hpp
│   ├── packer.hpp
│   ├── quest_graph.hpp
│   ├── validation_result.hpp
│   └── validator.hpp
├── src/
│   ├── main.cpp
│   ├── cli.cpp
│   ├── data_loader.cpp
│   ├── diagnostic.cpp
│   ├── json_reporter.cpp
│   ├── packer.cpp
│   ├── quest_graph.cpp
│   ├── validation_result.cpp
│   └── validator.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── cli_tests.cpp
│   ├── data_loader_tests.cpp
│   ├── packer_tests.cpp
│   ├── quest_graph_tests.cpp
│   ├── validator_tests.cpp
│   └── test_helpers/
│       └── temp_dir.hpp
├── sample_data/
│   ├── valid/
│   └── invalid/
│       ├── duplicate_id/
│       ├── missing_reference/
│       ├── missing_localization/
│       ├── quest_cycle/
│       ├── unreachable_quest/
│       ├── invalid_numbers/
│       ├── unused_localization/
│       └── malformed_json/
├── docs/
│   └── architecture.md
├── .github/workflows/ci.yml
└── .vscode/
    ├── extensions.json
    ├── launch.json
    ├── settings.json
    └── tasks.json
```

---

## Build Prerequisites

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| CMake | 3.21 | Downloads nlohmann/json and Catch2 via FetchContent |
| C++ compiler | MSVC 2019+, GCC 9+, or Clang 10+ | C++17 required |
| Internet access | — | Required for the first build (FetchContent downloads) |
| Git | Any | Required by CMake FetchContent |

---

## Windows Build Instructions

Open a Developer Command Prompt for VS 2022 (or ensure MSVC is on the PATH).

```cmd
git clone https://github.com/your-username/GameDataGuard.git
cd GameDataGuard

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

build\Release\gamedataguard.exe validate sample_data\valid
```

---

## Linux Build Instructions

```bash
git clone https://github.com/your-username/GameDataGuard.git
cd GameDataGuard

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

./build/gamedataguard validate sample_data/valid
```

---

## Visual Studio Code Instructions

1. Install the recommended extensions (VS Code will prompt you from `.vscode/extensions.json`).
2. Open the repository folder in VS Code.
3. Use **Ctrl+Shift+P → CMake: Configure** to configure the project.
4. Use **Ctrl+Shift+B** to build (default build task).
5. Use the **Run and Debug** panel to launch with the pre-configured debug configuration.
6. Use **Terminal → Run Task** to access the additional tasks (test, validate, build pack).

---

## CLI Usage

```
gamedataguard validate <data_directory> [options]
gamedataguard build    <data_directory> <output_pack> [options]
gamedataguard help
gamedataguard --help | -h

Options:
  --report <path>      Write a JSON diagnostic report to <path>.
  --warnings-as-errors Treat warnings as errors (exit code 1 on any warning).
```

**Exit codes:**

| Code | Meaning |
|------|---------|
| 0 | Success (validation passed, or pack built) |
| 1 | Validation failure (errors present, or warnings with --warnings-as-errors) |
| 2 | Usage error, missing/unreadable file, malformed JSON, or internal failure |

---

## Input Data Format

### Directory layout

```
data_directory/
├── manifest.json
├── npcs.json
├── items.json
├── quests.json
└── localization/
    ├── en.json
    ├── ja.json
    └── he.json
```

### manifest.json

```json
{
  "schema_version": 1,
  "required_locales": ["en", "ja", "he"],
  "start_quest_ids": ["quest_intro"]
}
```

### npcs.json

Root is a JSON array.

```json
[
  {
    "id": "npc_guide",
    "name_key": "npc.guide.name",
    "max_hp": 100,
    "attack": 10,
    "defense": 5
  }
]
```

### items.json

Root is a JSON array.

```json
[
  {
    "id": "item_potion",
    "name_key": "item.potion.name",
    "description_key": "item.potion.description",
    "buy_price": 50,
    "sell_price": 20
  }
]
```

### quests.json

Root is a JSON array.

```json
[
  {
    "id": "quest_intro",
    "title_key": "quest.intro.title",
    "description_key": "quest.intro.description",
    "giver_npc_id": "npc_guide",
    "reward_item_ids": ["item_potion"],
    "next_quest_ids": ["quest_second"]
  }
]
```

### Localization files

```json
{
  "npc.guide.name": "Guide",
  "item.potion.name": "Potion"
}
```

---

## Validation Rules

| Area | Rule | Code |
|------|------|------|
| Manifest | schema_version must equal 1 | GDG002 |
| Manifest | required_locales must be non-empty, unique | GDG004, GDG005 |
| Manifest | start_quest_ids must be non-empty | GDG004 |
| Manifest | each start quest must exist | GDG016 |
| NPCs | id and name_key required, non-empty | GDG004 |
| NPCs | id must be unique | GDG005 |
| NPCs | max_hp: 1–999999; attack/defense: 0–999999 | GDG008 |
| Items | id, name_key, description_key required | GDG004 |
| Items | id must be unique | GDG005 |
| Items | buy_price and sell_price ≥ 0 | GDG008 |
| Items | sell_price > buy_price is a warning | GDG012 |
| Quests | id, title_key, description_key required | GDG004 |
| Quests | id must be unique | GDG005 |
| Quests | giver_npc_id must reference an existing NPC | GDG006 |
| Quests | all reward_item_ids must reference existing items | GDG006 |
| Quests | all next_quest_ids must reference existing quests | GDG006 |
| Quests | no duplicate reward_item_ids | GDG007 |
| Quests | no duplicate next_quest_ids | GDG007 |
| Quests | no self-reference in next_quest_ids | GDG017 |
| Quests | no cycles in the quest graph | GDG013 |
| Quests | every quest reachable from a start quest | GDG014 |
| Localization | all required locale files must exist | GDG009 |
| Localization | all referenced keys must exist in every locale | GDG010 |
| Localization | localization values must not be empty/whitespace | GDG011 |
| Localization | unused localization keys produce warnings | GDG015 |

---

## Diagnostic Code Table

| Code | Severity | Description |
|------|----------|-------------|
| GDG001 | Error | Missing required file |
| GDG002 | Error | Unsupported schema version |
| GDG003 | Error | Malformed or invalid field type |
| GDG004 | Error | Empty required value |
| GDG005 | Error | Duplicate entity ID |
| GDG006 | Error | Missing entity reference |
| GDG007 | Error | Duplicate reference within a list |
| GDG008 | Error | Invalid numeric range |
| GDG009 | Error | Missing localization file |
| GDG010 | Error | Missing localization key |
| GDG011 | Error | Empty or whitespace-only localization value |
| GDG012 | Warning | Suspicious economic value (sell > buy) |
| GDG013 | Error | Quest graph cycle detected |
| GDG014 | Error | Unreachable quest |
| GDG015 | Warning | Unused localization key |
| GDG016 | Error | Invalid start quest ID |
| GDG017 | Error | Quest references itself |

---

## Asset Pack Binary Format

The `.gdgpack` file has the following layout:

| Offset | Size | Type | Value |
|--------|------|------|-------|
| 0 | 4 bytes | ASCII | Magic: `GDG1` |
| 4 | 4 bytes | uint32_t LE | Format version: `1` |
| 8 | 8 bytes | uint64_t LE | Payload size in bytes |
| 16 | N bytes | UTF-8 | JSON payload |

The JSON payload contains:

```json
{
  "manifest": { ... },
  "npcs": [ ... ],
  "items": [ ... ],
  "quests": [ ... ],
  "localization": {
    "en": { ... },
    "ja": { ... }
  }
}
```

**Determinism guarantees:**
- NPCs, items, and quests are sorted by `id`
- `required_locales` and `start_quest_ids` are sorted
- `reward_item_ids` and `next_quest_ids` within each quest are sorted
- Localization locale names and keys are emitted in sorted order (`std::map` order)
- Integer fields use little-endian byte order
- Payload is not compressed

The pack is written to a temporary file first and atomically renamed to the final path.

---

## Testing Instructions

```bash
# Configure and build (Debug recommended for tests)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel

# Run all tests
ctest --test-dir build --output-on-failure

# Run with sanitizers (GCC/Clang)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGDG_ENABLE_SANITIZERS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `GDG_WARNINGS_AS_ERRORS` | OFF | Treat all compiler warnings as errors |
| `GDG_ENABLE_SANITIZERS` | OFF | Enable AddressSanitizer + UBSan (GCC/Clang only) |
| `GDG_BUILD_UNITY_PLUGIN` | OFF | Build `gamedataguard_unity.dll` — Windows only |

---

## Unity Editor Integration

### Purpose

The Unity integration lets designers run GameDataGuard validation directly from
inside the Unity Editor without leaving their workflow. The same C++ validation
core used by the CLI is invoked through a native Windows DLL.

### Architecture

```
Unity EditorWindow  (Tools > GameDataGuard)
        │
        ▼  C# P/Invoke
C ABI wrapper  (gamedataguard_unity.dll)
        │
        ▼
gamedataguard_core  (static library)
```

`gamedataguard_unity.dll` is a thin bridge that calls `load_game_data()` and
`validate()` from `gamedataguard_core`, serialises the result to a UTF-8 JSON
string, and returns it through a C-compatible API. No validation logic is
duplicated; the Unity layer is presentation only.

### Scope

| Supported | Not supported |
|-----------|---------------|
| Windows x86-64 | macOS, Linux |
| Unity Editor | Runtime player |
| Directory validation | Runtime gameplay |

> **Screenshot location:** `docs/screenshots/unity_window.png`
> *(Add a real screenshot here after performing the manual smoke test.)*

### Prerequisites

- CMake 3.21+
- MSVC 2019 or later (Visual Studio 2019 / 2022)
- Unity 6 (6000.x), Editor only

### Build the Native DLL

```powershell
# From the repository root
cmake -B build -DGDG_BUILD_UNITY_PLUGIN=ON
cmake --build build --config Release --target gamedataguard_unity
```

The DLL is produced at `build/Release/gamedataguard_unity.dll`.

### Copy the DLL into the Unity Project

```powershell
.\integrations\unity\scripts\copy_plugin.ps1
# or for Debug: .\integrations\unity\scripts\copy_plugin.ps1 -Configuration Debug
```

The script copies the DLL to
`integrations/unity/GameDataGuardUnity/Assets/Plugins/x86_64/`.

> **Important:** Close Unity before running this script.
> Unity holds a file lock on the loaded DLL; replacing it while Unity is open
> will fail. After copying, reopen Unity to load the new version.

### Plugin Inspector Configuration

After copying the DLL, configure it in the Unity Plugin Inspector:

1. In the Project window, select
   `Assets/Plugins/x86_64/gamedataguard_unity.dll`.
2. In the Inspector:
   - **Any Platform** → unchecked
   - **Editor** → checked
   - **Platform** → Windows, x86_64
   - All other platforms → unchecked
3. Click **Apply**.

Unity generates a `.meta` file that records this configuration. Commit the
`.meta` file so team members do not need to reconfigure manually.

### Opening the Validation Window

```
Unity menu bar → Tools → GameDataGuard
```

### Using the Window

1. Click **Select Directory** and choose a directory containing `manifest.json`,
   `npcs.json`, `items.json`, `quests.json`, and a `localization/` subfolder.
2. Click **Validate**.
3. The window displays: status, error count, warning count, and a scrollable
   list of structured diagnostics (severity, code, file, JSON path, message).
4. Click **Clear** to reset.

### How Native Memory Works

`gdg_validate_directory_utf8` allocates a UTF-8 JSON string on the native heap.
The C# wrapper releases it through `gdg_free_string` in a `finally` block,
ensuring the buffer is freed even if JSON parsing fails. No native memory leaks
into the garbage collector.

### Why a C ABI

Unity P/Invoke requires stable C linkage. C++ names are mangled and object
layouts are compiler-specific. A plain `extern "C"` layer with fixed-width
integer types and UTF-8 `char*` parameters provides a stable, version-independent
surface that any C# runtime can call.

### How Failures are Represented

| Scenario | `status` | Return code |
|----------|----------|-------------|
| Validation passed | `"passed"` | 0 |
| Validation errors found | `"validation_failed"` | 1 |
| Missing file, malformed JSON, or internal error | `"tool_error"` | 2 |

Tool errors include a human-readable `message` field shown in the window. All
C++ exceptions are caught at the DLL boundary and converted to `tool_error`
results; the Unity Editor process is never at risk from a native exception.

### CMake Option

| Option | Default | Description |
|--------|---------|-------------|
| `GDG_BUILD_UNITY_PLUGIN` | OFF | Build `gamedataguard_unity.dll` (Windows only) |

### Manual Smoke Test Checklist

Perform these steps after building and copying the DLL.
These cannot be automated without a licensed Unity installation.

- [ ] 1. Unity opens without compilation errors.
- [ ] 2. `Tools > GameDataGuard` appears in the menu bar.
- [ ] 3. The GameDataGuard window opens.
- [ ] 4. The native DLL loads without a `DllNotFoundException`.
- [ ] 5. `sample_data/valid` → status PASSED, 0 errors.
- [ ] 6. `sample_data/invalid/missing_reference` → GDG006 diagnostic visible.
- [ ] 7. `sample_data/invalid/quest_cycle` → GDG013 diagnostic visible.
- [ ] 8. A nonexistent path → readable tool-error message in the window.
- [ ] 9. Running Validate a second time on the same directory works correctly.
- [ ] 10. After rebuilding the DLL and restarting Unity, the new version loads.

### Current Limitations

- Windows x86-64 and Unity Editor only; no runtime player support.
- The DLL must be copied manually after each rebuild using `copy_plugin.ps1`.
- Unity must be closed before replacing a loaded DLL.
- No `.meta` file for the plugin is committed until Unity has been opened and
  the Plugin Inspector has been configured and saved at least once.

---

## CI and Jenkins

### GitHub Actions

The `.github/workflows/ci.yml` workflow runs on every push and pull request across:

- `ubuntu-latest` with GCC (Release, warnings-as-errors)
- `ubuntu-latest` with GCC + sanitizers (Debug)
- `windows-latest` with MSVC (Release, warnings-as-errors)

Each job configures, builds, runs the test suite, validates `sample_data/valid`, and builds a sample pack. Artifacts (report JSON, `.gdgpack`) are uploaded for inspection.

### Jenkinsfile

The `Jenkinsfile` demonstrates a declarative pipeline with stages for Checkout, Configure, Build, Test, Validate Sample Data, Package Sample Data, and Archive Artifacts. It detects Unix vs. Windows agents via `isUnix()`.

---

## Design Decisions

1. **Separate validation from output.** The validator returns a `ValidationResult` struct. All output formatting happens in `main.cpp`. This makes testing much cleaner.

2. **`LoadResult` as `std::variant`.** Fatal loading errors (missing files, malformed JSON) are represented as `LoadError` rather than exceptions or error codes. This makes the control flow explicit at the call site.

3. **`std::map` for localization.** Localization data uses `std::map<string, string>` instead of `unordered_map` to guarantee sorted iteration order in the packer payload.

4. **Atomic pack writing.** The packer writes to a `.tmp` file and renames it atomically to the final path. This avoids leaving a partially-written pack if the process is interrupted.

5. **Deterministic cycle path reporting.** The DFS stack tracks the current path so that cycle messages include the full path string (e.g., `quest_a -> quest_b -> quest_a`).

6. **No exceptions across component boundaries.** Errors propagate as structured return types (`std::variant`, `std::optional`). Internal JSON parsing uses try/catch only at the point of `json::parse()`.

7. **No global mutable state.** All state is passed explicitly through function parameters or owned by local objects.

---

## Known Limitations

- Localization files must be listed in `manifest.json`. Extra locale files not listed there are silently ignored.
- The pack format does not support incremental updates; a full rebuild is required each time.
- The cycle detection reports the first cycle found in DFS order; if multiple independent cycles exist, additional passes may find others.
- Windows filenames with non-ASCII characters may have path-handling edge cases with the current `std::filesystem` usage.

---

## Potential Future Improvements

- Schema versioning with migration support
- Incremental validation (only re-validate changed files)
- Plugin system for custom validation rules
- A `diff` command to compare two pack files
- Compressed payload option (zstd or lz4)
- JSON Schema validation for input files
- Parallel loading of localization files
