from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class CBudgetTracker(ConanFile):
    name = "c_budget_tracker"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("gtest/1.14.0")
        self.requires("cmocka/1.1.8")
        # Criterion is NOT in Conan Center — install via: brew install criterion
        # CMakeLists.txt finds it automatically from /opt/homebrew.

    # NOTE: cmake_layout() is intentionally omitted.
    # With cmake_layout(), Conan generates CMakeUserPresets.json with configure
    # presets named "conan-debug" / "conan-release" using Unix Makefiles + a
    # nested build/build/<Type>/ directory — conflicting with our Ninja
    # Multi-Config presets in CMakePresets.json.
    # Without cmake_layout(), Conan writes conan_toolchain.cmake directly into
    # --output-folder (build/), matching the toolchainFile path in CMakePresets.json.
