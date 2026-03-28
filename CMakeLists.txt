cmake_minimum_required(VERSION 3.21)
project(c_budget_tracker C CXX)
include(CTest)

# ─── Ninja Multi-Config + Conan compatibility ─────────────────────────────────
# Conan generates per-config intermediate targets (e.g. CONAN_LIB::foo_RELEASE).
# CMake requires IMPORTED_LOCATION for every active configuration on every
# imported target. A _RELEASE target has no Debug location and vice versa.
# These mappings tell CMake which config to fall back to when the specific one
# is missing, silencing the "IMPORTED_LOCATION not set" errors.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG          "Debug;Release")
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE        "Release;Debug")
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RelWithDebInfo;Release;Debug")
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL     "MinSizeRel;Release;Debug")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Auto-create compile_commands.json symlink at project root so clangd finds it
# without a manual ln -sf. Runs at configure time (every cmake --preset <x>).
# Uses a relative path so the symlink works regardless of which preset was used:
#   cmake --preset base     → compile_commands.json → build/compile_commands.json
#   cmake --preset coverage → compile_commands.json → build-coverage/compile_commands.json
file(RELATIVE_PATH _build_rel ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
file(REMOVE ${CMAKE_SOURCE_DIR}/compile_commands.json)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${_build_rel}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# CXX needed for GTest test files (extern "C" wrapping pure C headers)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Coverage ────────────────────────────────────────────────────────────────
option(ENABLE_COVERAGE "Enable llvm-cov instrumentation" OFF)

function(enable_coverage target)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()
    target_compile_options(${target} PRIVATE
        -fprofile-instr-generate -fcoverage-mapping -O0 -g)
    target_link_options(${target} PRIVATE -fprofile-instr-generate)
endfunction()

# ─── Sanitizers ──────────────────────────────────────────────────────────────
# Enabled via ENABLE_SANITIZERS option (see CMakePresets.json sanitize preset).
# Cannot be combined with coverage in the same build.
option(ENABLE_SANITIZERS "Enable ASan + UBSan" OFF)

function(enable_sanitizers target)
    if(NOT ENABLE_SANITIZERS)
        return()
    endif()
    target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()

# ─── Dependencies ────────────────────────────────────────────────────────────
# GTest + cmocka: managed by Conan 2.x.
# Run `conan install . --output-folder=build --build=missing` before cmake.
find_package(GTest CONFIG REQUIRED)
find_package(cmocka CONFIG REQUIRED)

# Criterion: not in Conan Center — installed via Homebrew (brew install criterion).
# Search standard paths + Homebrew on Apple Silicon and Intel.
find_path(CRITERION_INCLUDE_DIR criterion/criterion.h
    HINTS /opt/homebrew/include /usr/local/include)
find_library(CRITERION_LIB criterion
    HINTS /opt/homebrew/lib /usr/local/lib)
if(CRITERION_INCLUDE_DIR AND CRITERION_LIB)
    message(STATUS "Criterion found: ${CRITERION_LIB}")
    set(CRITERION_FOUND TRUE)
else()
    message(STATUS "Criterion not found — skipping criterion tests. Install with: brew install criterion")
    set(CRITERION_FOUND FALSE)
endif()

# ─── Source library ──────────────────────────────────────────────────────────
# budget_core: everything except validators — used by the cmocka mock test so
# the test file can supply its own validate_amount / validate_category_name
# stubs without a multiple-definition linker error.
add_library(budget_core STATIC
    src/errors.c
    src/budget.c
    src/reports.c
)
enable_coverage(budget_core)
enable_sanitizers(budget_core)
target_include_directories(budget_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(budget_core PUBLIC m)

# budget_lib: full library (core + validators) — used by GTest and Criterion.
add_library(budget_lib STATIC src/validators.c)
enable_coverage(budget_lib)
enable_sanitizers(budget_lib)
target_include_directories(budget_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(budget_lib PUBLIC budget_core)

# ─── Test executables ────────────────────────────────────────────────────────

# 1. GTest (.cpp files with extern "C" wrapping pure C headers)
#    neotest-gtest handles individual test navigation from .cpp files.
#    Registered as one CTest test (not gtest_discover_tests) to avoid
#    neotest-ctest duplication.
add_executable(tests_gtest
    tests/gtest/test_validators.cpp
    tests/gtest/test_budget.cpp
    tests/gtest/test_reports.cpp
)
enable_coverage(tests_gtest)
enable_sanitizers(tests_gtest)
target_link_libraries(tests_gtest PRIVATE budget_lib GTest::gtest_main)
add_test(NAME gtest_suite COMMAND tests_gtest)

# 2. Criterion (pure C, BDD-style, no main() needed — requires brew install criterion)
if(CRITERION_FOUND)
    add_executable(tests_criterion
        tests/criterion/test_validators.c
        tests/criterion/test_budget.c
        tests/criterion/test_reports.c
    )
    enable_coverage(tests_criterion)
    enable_sanitizers(tests_criterion)
    target_include_directories(tests_criterion PRIVATE ${CRITERION_INCLUDE_DIR})
    target_link_libraries(tests_criterion PRIVATE budget_lib ${CRITERION_LIB})
    add_test(NAME criterion_suite COMMAND tests_criterion)
endif()

# 3a. cmocka validators (pure C, real validators — setup/teardown pattern)
add_executable(tests_cmocka_validators
    tests/cmocka/test_validators.c
)
enable_coverage(tests_cmocka_validators)
enable_sanitizers(tests_cmocka_validators)
target_link_libraries(tests_cmocka_validators PRIVATE budget_lib cmocka::cmocka)
add_test(NAME cmocka_validators COMMAND tests_cmocka_validators)

# 3b. cmocka mocked (pure C, link against budget_core only — test file supplies
#     its own validate_amount / validate_category_name mock via cmocka's mock())
add_executable(tests_cmocka_mocked
    tests/cmocka/test_budget_mocked.c
)
enable_coverage(tests_cmocka_mocked)
enable_sanitizers(tests_cmocka_mocked)
target_link_libraries(tests_cmocka_mocked PRIVATE budget_core cmocka::cmocka)
add_test(NAME cmocka_mocked COMMAND tests_cmocka_mocked)

# ─── Demo binary ─────────────────────────────────────────────────────────────
add_executable(budget_demo src/main.c)
enable_coverage(budget_demo)
enable_sanitizers(budget_demo)
target_link_libraries(budget_demo PRIVATE budget_lib)

# ─── Coverage report target ──────────────────────────────────────────────────
if(ENABLE_COVERAGE)
    if(CRITERION_FOUND)
        set(CRITERION_OBJ_ARGS "-object" "${CMAKE_BINARY_DIR}/Debug/tests_criterion")
    else()
        set(CRITERION_OBJ_ARGS)
    endif()

    # NOTE: Ninja wraps every custom-target COMMAND in `sh -c "..."`, so shell
    # redirection (>) works here. The regex is single-quoted so that `|` is not
    # interpreted as a pipe operator by the shell. Same pattern as CPP/CMakeLists.txt.
    add_custom_target(coverage-report
        COMMAND xcrun llvm-profdata merge -sparse
            ${CMAKE_BINARY_DIR}/Debug/*.profraw
            -o ${CMAKE_BINARY_DIR}/coverage.profdata
        COMMAND xcrun llvm-cov export
            ${CMAKE_BINARY_DIR}/Debug/tests_gtest
            ${CRITERION_OBJ_ARGS}
            -object ${CMAKE_BINARY_DIR}/Debug/tests_cmocka_validators
            -object ${CMAKE_BINARY_DIR}/Debug/tests_cmocka_mocked
            -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata
            -format=lcov
            -ignore-filename-regex='.*/.conan2/.*|.*/tests/.*'
            > ${CMAKE_SOURCE_DIR}/coverage/lcov.info
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/Debug/*.profraw
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating combined lcov..."
    )
endif()

# ─── Format targets ──────────────────────────────────────────────────────────
file(GLOB_RECURSE ALL_SOURCES
    "include/*.h"
    "src/*.c"
    "tests/gtest/*.cpp"
    "tests/criterion/*.c"
    "tests/cmocka/*.c"
)
add_custom_target(format
    COMMAND clang-format -i ${ALL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting all sources..."
)
add_custom_target(check-format
    COMMAND clang-format --dry-run ${ALL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking format..."
)
