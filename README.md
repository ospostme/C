# C Budget Tracker — Lab Notes

## Table of Contents

- [Part 1 — Background](#part-1--background-read-this-first)
  - [1.1 C vs C++ — What's Different](#11-c-vs-c--whats-different)
  - [1.2 Header / Implementation Split](#12-header--implementation-split)
  - [1.3 No RAII — Explicit Memory Management](#13-no-raii--explicit-memory-management)
  - [1.4 Dependency Management with Conan 2.x](#14-dependency-management-with-conan-2x)
  - [1.5 Three Test Frameworks Side-by-Side](#15-three-test-frameworks-side-by-side)
  - [1.6 Neovim Plugins for C Dev](#16-neovim-plugins-for-c-dev)
- [Part 2 — Project Reference](#part-2--project-reference)
- [Part 3 — Workflows](#part-3--workflows)
  - [Workflow 1 — First Build](#workflow-1--first-build)
  - [Workflow 2 — Run Tests](#workflow-2--run-tests)
  - [Workflow 3 — Neotest in Neovim](#workflow-3--neotest-in-neovim)
  - [Workflow 4 — Coverage](#workflow-4--coverage)
  - [Workflow 5 — Sanitizers](#workflow-5--sanitizers)
  - [Workflow 6 — Debug with codelldb](#workflow-6--debug-with-codelldb)
  - [Workflow 7 — clangd LSP](#workflow-7--clangd-lsp)
- [Part 4 — Bug Fixing Guide](#part-4--bug-fixing-guide)
- [Part 5 — Command Reference](#part-5--command-reference)

---

## Part 1 — Background (read this first)

### 1.1 C vs C++ — What's Different

| Dimension | C++ lab (CPP/) | C lab (c/) |
|---|---|---|
| Build | CMake + vcpkg | CMake + Conan 2.x |
| LSP | clangd | clangd (same binary) |
| Test framework | GTest only | GTest + Criterion + cmocka |
| Neotest adapter | neotest-gtest | neotest-gtest + neotest-ctest |
| Debugger | codelldb | codelldb (same) |
| Coverage | llvm-cov → lcov.info | llvm-cov → lcov.info |
| Sanitizers | optional | ASan + UBSan (more critical — no RAII) |
| Formatting | clang-format | clang-format (same) |
| Dependency manager | vcpkg | Conan 2.x |

Key reason for Conan here: **GTest and cmocka are in Conan Center; Criterion is NOT** — it must be installed via `brew install criterion`. This is actually the reason the CMakeLists.txt finds Criterion via `find_path()`/`find_library()` from `/opt/homebrew` rather than `find_package()`.

### 1.2 Header / Implementation Split

C requires explicit separation between declaration and definition:

```
src/
├── validators.h    ← function signatures (the contract — included by callers)
├── validators.c    ← function implementations
├── budget.h        ← struct definitions + function signatures
├── budget.c        ← implementations
...
```

**Include guards** (`#ifndef VALIDATORS_H / #define VALIDATORS_H / #endif`) prevent multiple inclusion when several `.c` files include the same header. This is the C equivalent of Rust's module system — but manual.

Unlike C++, there are **no classes, no namespaces, no method syntax**. Everything is a free function that takes a pointer to the struct:

```c
// C++ style (not valid C):
budget.add_transaction(...)

// C style:
budget_add_transaction(&budget, ...)
```

### 1.3 No RAII — Explicit Memory Management

C has no constructors, destructors, or smart pointers. This lab avoids dynamic allocation (`malloc`/`free`) entirely — all data lives in fixed-size arrays inside `Budget`. This is common in embedded C.

**Why sanitizers matter more in C than C++**: C++ RAII means destructors run automatically. In C, you must track every allocation manually. ASan (AddressSanitizer) catches:
- Use after free
- Stack buffer overflow
- Heap buffer overflow

UBSan (UndefinedBehaviorSanitizer) catches:
- Integer overflow
- Null pointer dereference
- Signed shift overflow

Enable with the `sanitize` CMake preset — not combinable with coverage.

### 1.4 Dependency Management with Conan 2.x

Conan 2.x is the package manager for this lab (vs vcpkg in the C++ lab):

```bash
# One-time setup: install all deps + generate CMake toolchain.
# Run TWICE (Debug + Release) — Ninja Multi-Config needs both so that
# Conan's imported targets expose includes for both configurations.
conan install . --output-folder=build --build=missing -s build_type=Debug
conan install . --output-folder=build --build=missing -s build_type=Release

# This generates:
#   build/conan_toolchain.cmake   ← CMake toolchain
#   build/conan-*.cmake files     ← find_package helpers for each dep
#   build/CMakeUserPresets.json   ← preset overrides (gitignored)
```

**Three dependency tiers — which tool handles each:**

| Tier | Example | How |
|---|---|---|
| OS / system | pthread, math (`-lm`) | `find_package(Threads)` in CMake |
| Conan packages | GTest, cmocka | `conanfile.py` → `conan install` → `find_package()` |
| Homebrew | Criterion | `brew install criterion` → found via `find_path()`/`find_library()` in CMakeLists.txt |
| Vendored | — | Not used in this lab |

### 1.5 Three Test Frameworks Side-by-Side

The same source bugs are tested by three frameworks to show how each reports failures:

**GTest** (`tests/gtest/*.cpp` — C++ syntax, `extern "C"` to call C):
```cpp
extern "C" { #include "validators.h" }

TEST(Validators, NegativeAmountShouldBeInvalid) {
    EXPECT_EQ(validate_amount(-50.0), BUDGET_ERR_INVALID_AMOUNT);
}
```
- Uses `EXPECT_*` (continue on failure) and `ASSERT_*` (abort on failure)
- `TEST_F(FixtureName, TestName)` for shared setup/teardown via `SetUp()`/`TearDown()`
- **Neotest**: handled by `neotest-gtest` — individual test case granularity

**Criterion** (`tests/criterion/*.c` — pure C, BDD style, no `main()`):
```c
Test(validators, negative_amount_should_be_invalid) {
    cr_assert_eq(validate_amount(-1.0), BUDGET_ERR_INVALID_AMOUNT,
                 "Negative amount must be rejected");
}
```
- No test registration required — Criterion discovers `Test()` via linker sections
- `TestSuite(name, .init = setup_fn)` for shared setup
- `cr_assert_eq` / `cr_assert_float_eq` / `cr_assert_neq`
- **Neotest**: handled by `neotest-ctest` — per-executable granularity (whole suite pass/fail)

**cmocka** (`tests/cmocka/*.c` — pure C, explicit registration, mocking):
```c
static void test_negative_amount_invalid(void **state) {
    (void)state;
    assert_int_equal(validate_amount(-10.0), BUDGET_ERR_INVALID_AMOUNT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_negative_amount_invalid),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```
- Requires explicit `main()` with test array — more ceremony but explicit
- `will_return(fn, value)` / `mock()` for function mocking (see `test_budget_mocked.c`)
- **Neotest**: handled by `neotest-ctest` — per-executable granularity

**Why the mocked binary is a separate executable**: `test_budget_mocked.c` provides its own `validate_amount` and `validate_category_name` implementations (mock stubs). Linking it against the full `budget_lib` (which includes `validators.c`) would cause a multiple-definition linker error. The CMakeLists.txt uses a `budget_core` library (without validators) specifically for the mock target.

### 1.6 Neovim Plugins for C Dev

| Plugin | Purpose |
|---|---|
| clangd (via mason) | LSP: completion, go-to-def, inline errors, format-on-save |
| neotest-gtest | Run/debug individual GTest `TEST()` cases from `.cpp` files |
| Overseer | Run Criterion + cmocka suites via `ctest --preset debug` (neotest-ctest is incompatible with this lab's `.c` file naming and frameworks) |
| codelldb | DAP debugger — same binary as Rust and C++ |
| clang-format | Formatting via `.clang-format` at project root |
| crazy-coverage.nvim | Inline lcov.info highlights in source files |

**clangd needs `compile_commands.json`**: the symlink at the project root is created automatically by CMake at configure time (`execute_process` in CMakeLists.txt). Any `cmake --preset <x>` recreates it — no manual `ln -sf` needed.

---

## Part 2 — Project Reference

```
c/
├── CMakeLists.txt           ← 3 test targets + coverage + sanitize functions
├── CMakePresets.json        ← configure/build/test/coverage/sanitize/workflow presets
├── conanfile.py             ← GTest + cmocka from Conan Center; Criterion via Homebrew
├── .clangd                  ← C standard, compile flags, -Iinclude fallback
├── .clang-format            ← LLVM style, 4-space indent, 100 col limit
├── compile_commands.json    ← symlink auto-created at configure time; points to whichever preset's binaryDir was last configured
├── coverage/                ← lcov.info output dir (gitignored)
├── include/                 ← public headers (declarations only)
│   ├── errors.h             ← BudgetError enum
│   ├── models.h             ← TransactionType, Category, Transaction structs
│   ├── validators.h         ← validate_amount, validate_category_name
│   ├── budget.h             ← Budget struct + function signatures
│   └── reports.h            ← report function signatures
├── src/                     ← implementations
│   ├── errors.c
│   ├── validators.c         ← [BUG #1]
│   ├── budget.c             ← [BUG #2, #3]
│   ├── reports.c            ← [BUG #4]
│   └── main.c               ← demo binary (not a test)
└── tests/
    ├── gtest/               ← .cpp files, extern "C", neotest-gtest
    │   ├── test_validators.cpp
    │   ├── test_budget.cpp
    │   └── test_reports.cpp
    ├── criterion/           ← pure C, BDD, neotest-ctest
    │   ├── test_validators.c
    │   ├── test_budget.c
    │   └── test_reports.c
    └── cmocka/              ← pure C, mocking, neotest-ctest
        ├── test_validators.c
        └── test_budget_mocked.c   ← mocks validate_amount / validate_category_name
```

**Four intentional bugs** (same domains as Rust and C++ labs):

| Bug | File | Description |
|---|---|---|
| #1 | `validators.c` | `validate_amount` always returns `BUDGET_OK` — no `amount > 0` check |
| #2 | `budget.c:budget_balance` | Adds expenses instead of subtracting |
| #3 | `budget.c:budget_category_over_limit` | Uses `>` instead of `>=` — at-limit not flagged |
| #4 | `reports.c:report_monthly_expenses` | Compares `tx->year == month` instead of `tx->year == year` |

---

## Part 3 — Workflows

### Workflow 1 — First Build

```bash
# 1. Install tools (one-time per machine)
pip install conan            # or: brew install conan
conan profile detect         # creates ~/.conan2/profiles/default
brew install criterion       # not in Conan Center — Homebrew only

cd c/

# 2. Install Conan deps (GTest + cmocka) + generate toolchain
#    Run twice — Ninja Multi-Config needs Debug and Release variants so that
#    Conan's imported targets expose includes for both configurations.
conan install . --output-folder=build --build=missing -s build_type=Debug
conan install . --output-folder=build --build=missing -s build_type=Release

# 3. Build the demo binary
# (The hidden 'base' configure preset runs automatically on first build;
#  compile_commands.json symlink is created by CMake at configure time.)
cmake --build --preset debug
./build/Debug/budget_demo
```

Expected output (with bugs active):
```
Balance:        4850.00    ← BUG #2: should be 1150.00 (3000 - 1500 - 200 - 150)
Total income:   3000.00
Total expenses: 1850.00
March expenses: 0.00       ← BUG #4: should be 1850.00
Food expenses:  350.00
Rent over limit: no        ← BUG #3: Rent==limit should be "yes"
Food over limit: no
```

### Workflow 2 — Run Tests

```bash
# Debug build (quickest for development)
cmake --build --preset debug
ctest --preset debug -V

# All three suites together (4 bugs → multiple failures across all frameworks)
ctest --preset debug -V --output-on-failure
```

CTest output shows three test suites:
```
1/4 Test #1: gtest_suite ................. Failed   (GTest: 4 failures)
2/4 Test #2: criterion_suite ............. Failed   (Criterion: 4 failures)
3/4 Test #3: cmocka_validators ........... Failed   (cmocka: 2 failures)
4/4 Test #4: cmocka_mocked ............... Failed   (cmocka: 1 failure)
```

Each framework reports the same bugs differently:
- GTest: `Expected: ... Which is: BUDGET_ERR_INVALID_AMOUNT` / `Actual: BUDGET_OK`
- Criterion: `Assertion failed: validate_amount(-1.0) == BUDGET_ERR_INVALID_AMOUNT`
- cmocka: `[test_negative_amount_invalid] FAILED`

### Workflow 3 — Neotest in Neovim

**Opening a GTest `.cpp` file** — neotest-gtest activates:

| Key | Action |
|---|---|
| `<leader>tn` | Run the nearest `TEST()` or `TEST_F()` under cursor |
| `<leader>tf` | Run all tests in the current `.cpp` file |
| `<leader>tt` | Run all tests (all three suites via CTest) |
| `<leader>td` | Debug nearest test via codelldb DAP |

**Opening a Criterion or cmocka `.c` file** — Overseer handles it:

`<leader>tn` or `<leader>tt` on a `.c` file runs `ctest --preset debug --output-on-failure` via Overseer, which shows all suite results in the Overseer window. neotest-ctest is not used here — it requires `*_test.cpp` naming and only supports GTest/Catch2/doctest, making it incompatible with this lab's Criterion and cmocka `.c` files.

**neotest-gtest does NOT parse `.c` files** — it only scans `.cpp` files for `TEST(` macros. Criterion and cmocka `.c` files always route to Overseer.

### Workflow 4 — Coverage

**From Neovim** (recommended):
```
<leader>Cc   →  runs cmake coverage workflow → produces coverage/lcov.info
<leader>tct  →  toggle inline coverage highlights in source
]cu / [cu    →  jump to next/previous uncovered line
```

**From terminal** (three-step):
```bash
# Step 1: configure + build with profiling instrumentation
cmake --preset coverage
cmake --build --preset coverage

# Step 2: run tests (profraw files written to build-coverage/Debug/*.profraw)
ctest --preset coverage --no-tests=ignore

# Step 3: merge profraw → combine → export lcov
cmake --build --preset coverage-report
# → coverage/lcov.info produced
```

**Or use the full workflow preset** (stops at first failure — use steps above if tests are broken):
```bash
cmake --workflow --preset coverage
```

**`<leader>Cc` vs the workflow preset**: The Neovim keymap splits configure+build and the coverage-report step so the lcov report is generated even when some tests fail (matching the Rust `<leader>Rc` behaviour). The workflow preset stops the entire sequence on test failure, which means no report if bugs are present.

**CMake coverage-report implementation note** (consistent with CPP/CMakeLists.txt):

Ninja wraps every `add_custom_target COMMAND` in `sh -c "..."` on Unix/macOS, so shell redirection (`>`) works inside CMake custom targets even though CMake itself does not use a shell. Two rules follow from this:

1. **Use `>` for llvm-cov output** — Apple LLVM 17's `llvm-cov export` has no `-o` flag; output goes to stdout. The `>` redirect captures it cleanly.
2. **Single-quote the `-ignore-filename-regex` value** — the regex uses `|` (alternation). Without quotes, the shell interprets `|` as a pipe operator and the token after it (`.*/tests/.*`) becomes a command, causing a *"is a directory"* error. Single quotes make `|` a literal character inside the regex.

```cmake
# Correct — same pattern used in both CPP/ and c/
-ignore-filename-regex='.*/.conan2/.*|.*/tests/.*'
> ${CMAKE_SOURCE_DIR}/coverage/lcov.info
```

### Workflow 5 — Sanitizers

```bash
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize -V

# Or workflow:
cmake --workflow --preset sanitize
```

ASan output example (stack buffer overflow):
```
==12345==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x...
```

UBSan output example (undefined behaviour):
```
runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
```

**Cannot combine sanitizers with coverage** — they use different compiler instrumentation and clash at link time. Use separate build dirs or presets.

### Workflow 6 — Debug with codelldb

1. Open `src/budget.c`
2. `\b` — set breakpoint on `budget_balance`
3. Open `tests/gtest/test_budget.cpp`, position cursor on `TEST_F(BudgetTest, BalanceSubtractsExpenses)`
4. `<leader>td` — neotest-gtest launches codelldb, hits the breakpoint in C code
5. `\s` — step into, `\n` — step over, `?` — inspect variable under cursor

codelldb handles C and C++ identically — same DAP config, same keymaps.

### Workflow 7 — clangd LSP

clangd is already installed (shared with the C++ lab). It reads `.clangd` at the project root:

```yaml
# .clangd
CompileFlags:
  Add:
    - -std=c11
    - -Wall
    - -Wextra
  CompilationDatabase: .   # reads compile_commands.json symlink at root
```

**LSP features in C**:
- `gd` — go to definition (jumps between `.h` and `.c`)
- `gr` — find all references
- `K` — hover docs (shows struct fields, function signatures)
- `<leader>rn` — rename symbol (updates all `.h` and `.c` usages)
- Format on save via clang-format (configured in `autoformat.lua`)

**No rust-analyzer conflict**: unlike Neovim 0.11's auto-start of `rust_analyzer`, clangd is only started by mason-lspconfig when a C/C++ file is opened. No `vim.lsp.enable("clangd", false)` override is needed.

---

## Part 4 — Bug Fixing Guide

Fix each bug and watch the test output change across all three frameworks simultaneously.

### Bug #1 — `validators.c:validate_amount` always returns OK

```c
// BUG: amount is never checked
BudgetError validate_amount(double amount) {
    (void)amount;
    return BUDGET_OK;
}

// FIX: add the guard
BudgetError validate_amount(double amount) {
    if (amount <= 0.0) {
        return BUDGET_ERR_INVALID_AMOUNT;
    }
    return BUDGET_OK;
}
```

After fix: GTest `Validators.NegativeAmountShouldBeInvalid` passes, Criterion `validators/negative_amount_should_be_invalid` passes, cmocka `test_negative_amount_invalid` passes.

### Bug #2 — `budget_balance` adds expenses instead of subtracting

```c
// BUG:
total += tx->amount;  // income and expense both add

// FIX:
if (tx->type == TRANSACTION_INCOME) {
    total += tx->amount;
} else {
    total -= tx->amount;
}
```

### Bug #3 — `budget_category_over_limit` uses `>` instead of `>=`

```c
// BUG: spending exactly at limit is not flagged
return spent > limit;

// FIX:
return spent >= limit;
```

### Bug #4 — `report_monthly_expenses` year filter is wrong

```c
// BUG: checks tx->year == month (compares year field to month value)
if (tx->type == TRANSACTION_EXPENSE && tx->year == month && tx->month == month)

// FIX:
if (tx->type == TRANSACTION_EXPENSE && tx->year == year && tx->month == month)
// Also remove: (void)year;
```

---

## Part 5 — Command Reference

### Conan

```bash
conan profile detect                          # detect default profile (run once)
conan install . --output-folder=build --build=missing -s build_type=Debug
conan install . --output-folder=build --build=missing -s build_type=Release
conan list "*"                                # list installed packages
```

### CMake

```bash
# Configure — only needed for special builds (coverage/sanitize have separate binaryDirs).
# Normal debug/release: the hidden 'base' preset runs automatically on first cmake --build.
cmake --preset coverage                  # configure build-coverage/ with ENABLE_COVERAGE=ON
cmake --preset sanitize                  # configure build-sanitize/ with ENABLE_SANITIZERS=ON

# Build (Debug and Release both live in build/ — select via --config)
cmake --build --preset debug             # → build/Debug/
cmake --build --preset release           # → build/Release/ (no reconfigure needed)
cmake --build --preset coverage          # → build-coverage/Debug/
cmake --build --preset sanitize          # → build-sanitize/Debug/
cmake --build --preset coverage-report   # generates coverage/lcov.info

# Test
ctest --preset debug -V
ctest --preset coverage --no-tests=ignore
ctest --preset sanitize -V

# Format
cmake --build --preset format                # clang-format -i all sources
cmake --build --preset check-format          # dry run check

# Workflows (stop on failure)
cmake --workflow --preset coverage
cmake --workflow --preset sanitize
```

### Running individual test binaries

```bash
# Debug binaries (cmake --build --preset debug)
./build/Debug/tests_gtest
./build/Debug/tests_criterion
./build/Debug/tests_cmocka_validators
./build/Debug/tests_cmocka_mocked

# Coverage binaries (cmake --build --preset coverage) — also fully debuggable (-g -O0)
./build-coverage/Debug/tests_gtest
./build-coverage/Debug/tests_criterion

# GTest: filter by test name
./build/Debug/tests_gtest --gtest_filter="Validators.*"
./build/Debug/tests_gtest --gtest_filter="BudgetTest.BalanceSubtractsExpenses"

# Criterion: run specific suite
./build/Debug/tests_criterion --filter "validators/*"

# CTest: run specific test by name
ctest --preset debug -R gtest_suite -V
ctest --preset debug -R criterion_suite -V
```

---

## Part 6 — Build System Tutorial

This section documents what Ninja Multi-Config actually means, why presets are designed the way they are, and why builds are isolated the way they are. This knowledge generalises to any CMake project using this generator.

### 6.1 What "Ninja Multi-Config" Means

Standard Ninja (single-config) bakes one build type into the build directory at configure time:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug    # Debug baked in — must delete and redo for Release
cmake --build build/
```

**Ninja Multi-Config** generates rules for ALL build types from a single configure step:

```bash
cmake --build --preset debug     # → build/Debug/
cmake --build --preset release   # → build/Release/   ← no reconfigure, just recompile
```

You can observe this directly after any `cmake --build --preset <x>`:

```
build/
├── build-Debug.ninja          ← rules for Debug
├── build-Release.ninja        ← rules for Release
├── build-RelWithDebInfo.ninja ← rules for RelWithDebInfo
├── build.ninja                ← dispatcher
├── Debug/                     ← binaries from --config Debug
├── Release/                   ← binaries from --config Release
└── RelWithDebInfo/            ← binaries from --config RelWithDebInfo
```

All three output directories are created from one configure step.

### 6.2 Configure Presets vs Build Configs

`CMAKE_BUILD_TYPE` is a **single-config** concept — it's ignored by Ninja Multi-Config. Setting it in a configure preset does nothing.

The correct mental model:

| Concept | What it represents | When it applies |
|---|---|---|
| Configure preset | Toolchain + special compiler flags | At `cmake --preset <x>` time |
| `--config Debug/Release` | Which output directory to build into | At `cmake --build` time |

Configure presets should represent structurally different builds: normal, coverage-instrumented, or sanitizer-enabled. Debug/Release are NOT configure presets — they're always available via `--config` from a single configure.

**This lab's presets follow this rule:**

| Configure preset | What it adds | binaryDir |
|---|---|---|
| `base` (hidden) | Nothing — normal build | `build/` |
| `coverage` | `-fprofile-instr-generate -fcoverage-mapping -O0` | `build-coverage/` |
| `sanitize` | `-fsanitize=address,undefined` | `build-sanitize/` |

`debug` and `release` are **build presets only** — they select `--config Debug` or `--config Release` from the `base` configure.

### 6.3 Why Coverage and Sanitize Use Separate binaryDirs

Coverage adds compiler flags (`-fprofile-instr-generate -fcoverage-mapping`) that are unknown to Ninja Multi-Config's built-in Debug/Release concept. They must be applied via CMake cache variables (`ENABLE_COVERAGE=ON`).

If coverage shared `build/` with debug:
- `cmake --preset coverage` would reconfigure `build/` with `ENABLE_COVERAGE=ON`
- CMake detects the flag change → marks all object files dirty → full recompile
- The previous debug binary is replaced — debug and coverage cannot coexist

With separate `binaryDir`:
```
build/            ← debug + release (cmake --build --preset debug/release)
build-coverage/   ← coverage instrumented (cmake --build --preset coverage)
build-sanitize/   ← ASan + UBSan (cmake --build --preset sanitize)
```

All three builds coexist. Switching between them costs nothing — no rebuild.

### 6.4 Coverage Binary Is Fully Debuggable

Coverage instrumentation flags:
```
-fprofile-instr-generate   ← emit profiling hooks
-fcoverage-mapping         ← embed source mapping metadata
-O0                        ← no optimisation (same as debug)
-g                         ← full debug symbols (same as debug)
```

`-O0 -g` are identical to a debug build. You can attach codelldb, set breakpoints, step through code, and inspect variables in a coverage binary — exactly like a debug binary.

Practical workflow:
```
<leader>Cc   →  builds build-coverage/, runs tests, generates coverage/lcov.info
<leader>tct  →  loads lcov highlights in the source file
\b           →  set a breakpoint in the same source file
<leader>td   →  debug the coverage binary — no rebuild needed
```

### 6.5 compile_commands.json and clangd

clangd reads `compile_commands.json` at the project root to understand include paths and compiler flags. CMake generates this file in each binaryDir.

The project root has a symlink that is recreated every time you configure:
```bash
cmake --build --preset debug    → compile_commands.json → build/compile_commands.json
cmake --preset coverage         → compile_commands.json → build-coverage/compile_commands.json
```

**For normal development**: configure (or build) with the `base` preset. The symlink points to `build/compile_commands.json` — this gives clangd clean flags without coverage or sanitizer instrumentation, which is what you want for IDE features.

**After running coverage**: clangd will briefly use `build-coverage/compile_commands.json` (with `-fprofile-instr-generate`). This is harmless — clangd ignores profiling flags. Run `cmake --build --preset debug` once to restore the symlink to `build/`.
