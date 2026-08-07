# Contributing to Gameplay Football

Thank you for taking the time to contribute!

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [How to Report a Bug](#how-to-report-a-bug)
3. [How to Request a Feature](#how-to-request-a-feature)
4. [Development Workflow](#development-workflow)
5. [Coding Style](#coding-style)
6. [Commit Messages](#commit-messages)
7. [Pull Request Guidelines](#pull-request-guidelines)

---

## Getting Started

1. Fork the repository and clone your fork.
2. Follow the [README](README.md) build instructions for your platform.
3. Create a feature branch from `master`:
   ```bash
   git checkout -b feature/my-improvement
   ```

---

## How to Report a Bug

> **Note:** the GitHub Issues tab is **not active** for this project, so bugs
> are best reported via a pull request. Include enough detail for us to
> reproduce the problem in your PR description:
> - Operating system and compiler version.
> - Steps to reproduce.
> - Expected vs. actual behaviour.
> - Any relevant logs or screenshots.

---

## How to Request a Feature

- Check the [ROADMAP](ROADMAP.md) – the item may already be planned.
- Open a pull request implementing the feature, or describe your proposal in a
  PR with a clear motivation and suggested approach.

---

## Development Workflow

```bash
# 1. Configure (from repo root)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# 2. Build (POST_BUILD copies runtime assets beside the binary)
cmake --build build --parallel
# 3. Run from the build directory
(cd build && ./gameplayfootball)
```

On Linux you can install packages with `scripts/setup_linux_deps.sh`.
On Windows use `scripts/setup_windows_deps.ps1` (vcpkg + `vcpkg.json`).

For iterative development, rebuild with `cmake --build build --parallel` after changes.

---

## Coding Style

The project uses **C++17** and the **Google C++ Style Guide** as a baseline,
enforced by [`.clang-format`](.clang-format).

Before committing, format changed files:

```bash
# Format a single file
clang-format -i src/my_file.cpp

# Format all C++ source files (from repo root)
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

Key rules:

- Use `std::` smart pointers (`std::unique_ptr`, `std::shared_ptr`) for new
  code; avoid raw owning pointers.
- Prefer `auto` where the type is obvious from context.
- Use `[[nodiscard]]` on functions whose return values must be checked.
- Keep functions short and focused (single responsibility).
- No trailing whitespace; Unix line endings (`\n`).

---

## Commit Messages

Follow the **Conventional Commits** convention:

```
<type>(<scope>): <short description>

[optional body]

[optional footer]
```

Common types: `feat`, `fix`, `refactor`, `docs`, `ci`, `test`, `chore`.

Examples:
```
feat(ai): add counter-press tactic
fix(ball): correct spin decay formula
docs(readme): update macOS build instructions
```

---

## Pull Request Guidelines

1. Target the `master` branch.
2. Keep PRs focused – one feature or fix per PR.
3. Make sure the build passes (CI must be green).
4. Update `ROADMAP.md` if your PR completes a roadmap item.
5. Add or update tests where applicable.
6. A maintainer will review and may request changes before merging.
