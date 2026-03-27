---
description: Modernize the League Soccer codebase to modern C++ standards
---
# Modernize Codebase Workflow

This workflow outlines steps to bring the **League Soccer** C++ codebase up to date with modern C++ (C++17/20) practices, improve safety, readability, and maintainability.

## Prerequisites
- Ensure `clang-format` and `clang-tidy` are installed and available in the system PATH.
- The project builds with CMake; we will enforce a minimum C++ standard.

## Steps
1. **Update CMake to require C++17**
   ```
   // In CMakeLists.txt, add or update:
   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_CXX_EXTENSIONS OFF)
   ```
   // turbo
2. **Run clang-tidy with automatic fixes**
   ```
   clang-tidy -p build $(git ls-files "*.cpp" "*.hpp") -fix -checks="-*,modernize-*"
   ```
   // turbo
3. **Apply clang-format to all source files**
   ```
   find d:/Games/League-Soccer/src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +
   ```
   // turbo
4. **Replace raw pointers with smart pointers**
   - Identify allocations using `new` and ownership semantics.
   - Replace with `std::unique_ptr` for exclusive ownership or `std::shared_ptr` where shared.
   - Update corresponding deletions (remove manual `delete`).
5. **Use `nullptr` instead of `NULL`/`0`**
   - Search for `NULL` or literal `0` used as pointer and replace with `nullptr`.
6. **Add `override` keyword to overridden virtual methods**
   - Search for methods overriding base class virtual functions and append `override`.
7. **Introduce `[[nodiscard]]` for functions returning error codes**
   - Add attribute to functions where ignoring the return value is likely a bug.
8. **Convert C‑style loops to range‑based for where appropriate**
   - Example: `for (auto &obj : container) { ... }`
9. **Use `auto` for type deduction where it improves readability**
   - Apply judiciously, avoiding loss of clarity.
10. **Enable compiler warnings and treat them as errors**
    - Add to CMake: `target_compile_options(your_target PRIVATE -Wall -Wextra -Werror)`
    
## Verification
- Rebuild the project: `cmake -S . -B build && cmake --build build`
- Run existing unit tests (if any) to ensure behavior unchanged.
- Perform static analysis (`clang-tidy` without `-fix`) to confirm no new warnings.

## Notes
- Steps marked with `// turbo` are safe to auto‑run via the `run_command` tool with `SafeToAutoRun: true`.
- Manual review is recommended after pointer refactoring to verify ownership semantics.
- Adjust the list of `clang-tidy` checks as needed for project‑specific conventions.
