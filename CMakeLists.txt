cmake_minimum_required(VERSION 3.21)

project(GameDataGuard
    VERSION 1.0.0
    DESCRIPTION "Game data validation and packaging tool"
    LANGUAGES CXX)

# ---------------------------------------------------------------
# Options
# ---------------------------------------------------------------
option(GDG_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(GDG_ENABLE_SANITIZERS  "Enable AddressSanitizer and UBSan (GCC/Clang only)" OFF)

# ---------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------
# Dependencies via FetchContent
# ---------------------------------------------------------------
include(FetchContent)

# nlohmann/json - pinned to a stable release
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Catch2 - pinned to a stable release
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# ---------------------------------------------------------------
# Compiler warning flags (per-target, not global)
# ---------------------------------------------------------------
function(gdg_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            $<$<BOOL:${GDG_WARNINGS_AS_ERRORS}>:/WX>
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            $<$<BOOL:${GDG_WARNINGS_AS_ERRORS}>:-Werror>
        )
    endif()
endfunction()

# ---------------------------------------------------------------
# Sanitizer flags
# ---------------------------------------------------------------
function(gdg_set_sanitizers target)
    if(GDG_ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined)
        else()
            message(WARNING "Sanitizers are only supported with GCC or Clang")
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------
# Core library target
# ---------------------------------------------------------------
add_library(gamedataguard_core STATIC
    src/diagnostic.cpp
    src/validation_result.cpp
    src/data_loader.cpp
    src/validator.cpp
    src/quest_graph.cpp
    src/json_reporter.cpp
    src/packer.cpp
    src/cli.cpp
)

target_include_directories(gamedataguard_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(gamedataguard_core PUBLIC
    nlohmann_json::nlohmann_json
)

gdg_set_warnings(gamedataguard_core)
gdg_set_sanitizers(gamedataguard_core)

# ---------------------------------------------------------------
# CLI executable
# ---------------------------------------------------------------
add_executable(gamedataguard src/main.cpp)

target_link_libraries(gamedataguard PRIVATE gamedataguard_core)

gdg_set_warnings(gamedataguard)
gdg_set_sanitizers(gamedataguard)

# ---------------------------------------------------------------
# Tests
# ---------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
