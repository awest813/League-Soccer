# Gameplay Football – Roadmap

This document tracks the modernisation work and planned new features for the
**League-Soccer / Gameplay Football** project.  Contributions are always
welcome – see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

---

## Status Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Done |
| 🔄 | In progress |
| 📋 | Planned |
| 💡 | Idea / under consideration |

---

## Phase 1 – Housekeeping & Modernisation

> *Goal: bring the build system, tooling and code quality up to contemporary
> standards so that contributors can work efficiently.*

| # | Task | Status |
|---|------|--------|
| 1.1 | Upgrade C++ standard from C++14 → **C++17** | ✅ |
| 1.2 | Bump `cmake_minimum_required` to **3.14** | ✅ |
| 1.3 | Switch to **target-based CMake** (`target_include_directories`, `target_link_libraries PRIVATE`) | ✅ |
| 1.4 | Enable `CMAKE_EXPORT_COMPILE_COMMANDS` for IDE / clangd support | ✅ |
| 1.5 | Add **`.clang-format`** (Google/C++17 style) | ✅ |
| 1.6 | Add **GitHub Actions CI** workflow (Ubuntu build) | ✅ |
| 1.7 | Add `CONTRIBUTING.md` | ✅ |
| 1.8 | Update `README.md` (fix URLs, add developer section) | ✅ |
| 1.9 | Replace `boost::shared_ptr` / `boost::intrusive_ptr` with `std::shared_ptr` where safe | 📋 |
| 1.10 | Replace `boost::thread` with `std::thread` | 📋 |
| 1.11 | Replace raw `new`/`delete` owners with `std::unique_ptr` | 📋 |
| 1.12 | Address suppressed compiler warnings one module at a time | 📋 |
| 1.13 | Enforce clang-format in CI | 📋 |
| 1.14 | Add **clang-tidy** configuration and CI step | 📋 |
| 1.15 | Windows CI (via MSVC / vcpkg) | 📋 |
| 1.16 | macOS CI (fix main-thread rendering issue) | 📋 |

---

## Phase 2 – Testing Infrastructure

> *Goal: make it possible to validate game logic without running the full
> graphical application.*

| # | Task | Status |
|---|------|--------|
| 2.1 | Introduce **Google Test** (or Catch2) as a CMake `FetchContent` dependency | 📋 |
| 2.2 | Headless / mock rendering mode so tests can run without a display | 📋 |
| 2.3 | Unit tests for `base/math` (Vector3, Matrix3, Quaternion, bluntmath) | 📋 |
| 2.4 | Unit tests for ball-physics calculations | 📋 |
| 2.5 | Unit tests for player-velocity state machine | 📋 |
| 2.6 | Integration test: simulate a full 90-second match and assert score/state | 📋 |
| 2.7 | Run tests in GitHub Actions CI | 📋 |

---

## Phase 3 – Gameplay Features

> *Goal: add commonly requested features and quality-of-life improvements.*

| # | Task | Status |
|---|------|--------|
| 3.1 | **Replay system** – record match state and play it back | 📋 |
| 3.2 | **Statistics overlay** – shots on target, possession %, pass accuracy | 📋 |
| 3.3 | **Save/Load match state** (SQLite-backed) | 📋 |
| 3.4 | **Improved AI** – add pressure/counter-press tactics | 📋 |
| 3.5 | **Set-piece editor** – configurable free-kick / corner routines | 📋 |
| 3.6 | **Player stamina system** – fatigue affecting sprint speed | 📋 |
| 3.7 | **Injury / substitution system** | 📋 |
| 3.8 | **Weather effects** – rain/wind altering ball trajectory | 💡 |
| 3.9 | **Dynamic crowd audio** – crowd reacts to chances and goals | 📋 |

---

## Phase 4 – Platform & UX

> *Goal: broaden platform support and improve the user experience.*

| # | Task | Status |
|---|------|--------|
| 4.1 | Fix macOS rendering (dispatch OpenGL calls to main thread) | 📋 |
| 4.2 | Windows CI/CD with automated release artifacts | 📋 |
| 4.3 | **HiDPI / Retina display** support | 📋 |
| 4.4 | **Resolution selector** in the options menu | 📋 |
| 4.5 | **Controller remapping** in-game menu | 📋 |
| 4.6 | **Localisation / i18n** scaffolding | 💡 |
| 4.7 | **Shader improvements** – normal mapping, ambient occlusion | 💡 |
| 4.8 | **Widescreen camera** preset (16:9 / 21:9) | 📋 |

---

## Phase 5 – Developer Experience

| # | Task | Status |
|---|------|--------|
| 5.1 | Generate API docs with **Doxygen** | 📋 |
| 5.2 | Architecture overview document | 📋 |
| 5.3 | Docker-based development environment | 📋 |
| 5.4 | Script to auto-fetch and set up data assets | 📋 |

---

## How to Contribute

Pick any `📋` item, open an issue to discuss, then send a pull request.
See [CONTRIBUTING.md](CONTRIBUTING.md) for coding style, branch naming and
PR process.
