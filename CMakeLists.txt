cmake_minimum_required(VERSION 3.21)

project(GameDataGuard
    VERSION 1.0.0
    DESCRIPTION "Game data validation and packaging tool"
    LANGUAGES CXX)

# ---------------------------------------------------------------
# Options
# ---------------------------------------------------------------
option(GDG_WARNINGS_AS_ERRORS  "Treat compiler warnings as errors" OFF)
option(GDG_ENABLE_SANITIZERS   "Enable AddressSanitizer and UBSan (GCC/Clang only)" OFF)
option(GDG_BUILD_UNITY_PLUGIN
    "Build the Windows x86-64 Unity Editor native plugin (gamedataguard_unity.dll)"
    OFF)

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
            /utf-8    # required on Japanese locale (code page 932) to read UTF-8 sources
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

# ---------------------------------------------------------------
# Unity Editor native plug-in (Windows x86-64 only)
# ---------------------------------------------------------------
if(GDG_BUILD_UNITY_PLUGIN)
    if(NOT WIN32)
        message(FATAL_ERROR
            "GDG_BUILD_UNITY_PLUGIN is only supported on Windows.")
    endif()

    add_library(gamedataguard_unity SHARED
        integrations/unity/native/gamedataguard_unity.cpp
    )

    target_include_directories(gamedataguard_unity PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/integrations/unity/native
    )

    # GDG_UNITY_EXPORTS activates __declspec(dllexport) in the header.
    target_compile_definitions(gamedataguard_unity PRIVATE GDG_UNITY_EXPORTS)

    # gamedataguard_core is PUBLIC on nlohmann_json, so the bridge inherits
    # the include directories automatically.
    target_link_libraries(gamedataguard_unity PRIVATE gamedataguard_core)

    set_target_properties(gamedataguard_unity PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

    gdg_set_warnings(gamedataguard_unity)

    message(STATUS
        "GameDataGuard: Unity plugin target enabled (gamedataguard_unity)")
endif()
