# Contributing to Gameplay Football

Thank you for taking the time to contribute! 🎉

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
3. Create a feature branch from `main`:
   ```bash
   git checkout -b feature/my-improvement
   ```

---

## How to Report a Bug

- Search existing [issues](../../issues) to avoid duplicates.
- Open a new issue and fill in the bug report template, including:
  - Operating system and compiler version.
  - Steps to reproduce.
  - Expected vs. actual behaviour.
  - Any relevant logs or screenshots.

---

## How to Request a Feature

- Check the [ROADMAP](ROADMAP.md) – the item may already be planned.
- Open an issue labelled **enhancement** with a clear description of the
  motivation and proposed solution.

---

## Development Workflow

```bash
# 1. Configure (from repo root)
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# 2. Build
make -j$(nproc)
# 3. Run
./gameplayfootball
```

For iterative development, rebuild with `make -j$(nproc)` after changes.

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

1. Target the `main` branch.
2. Keep PRs focused – one feature or fix per PR.
3. Make sure the build passes (CI must be green).
4. Update `ROADMAP.md` if your PR completes a roadmap item.
5. Add or update tests where applicable.
6. A maintainer will review and may request changes before merging.
