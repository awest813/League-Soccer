# Gameplay Football - Roadmap

This document tracks the modernization work and planned new features for the
**League-Soccer / Gameplay Football** project. Contributions are always
welcome - see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

---

## Status Legend

| Status | Meaning |
|--------|---------|
| `DONE` | Completed |
| `IN PROGRESS` | Work has started |
| `PLANNED` | Scheduled but not started |
| `IDEA` | Under consideration |

---

## Phase 1 - Housekeeping and Modernization

> *Goal: bring the build system, tooling and code quality up to contemporary
> standards so that contributors can work efficiently.*

| # | Task | Status |
|---|------|--------|
| 1.1 | Upgrade C++ standard from C++14 to **C++17** | `DONE` |
| 1.2 | Bump `cmake_minimum_required` to **3.14** | `DONE` |
| 1.3 | Switch to **target-based CMake** (`target_include_directories`, `target_link_libraries PRIVATE`) | `DONE` |
| 1.4 | Enable `CMAKE_EXPORT_COMPILE_COMMANDS` for IDE / clangd support | `DONE` |
| 1.5 | Add **`.clang-format`** (Google/C++17 style) | `DONE` |
| 1.6 | Add GitHub Actions CI workflow (Ubuntu build) | `DONE` |
| 1.7 | Add `CONTRIBUTING.md` | `DONE` |
| 1.8 | Update `README.md` (fix URLs, add developer section) | `DONE` |
| 1.9 | Replace `boost::bind` with lambdas / `std::bind`; `boost::random` with `std::random`; `boost::circular_buffer` with custom std impl | `DONE` |
| 1.10 | Replace `boost::thread` with `std::thread` | `DONE` |
| 1.11 | Replace raw owner patterns with RAII (`std::unique_ptr`) across core data / DB paths | `DONE` |
| 1.12 | Remove global warning suppressions and keep warnings visible during incremental cleanup | `DONE` |
| 1.13 | Enforce clang-format in CI (checks changed files per PR) | `DONE` |
| 1.14 | Add **clang-tidy** configuration and CI step | `DONE` |
| 1.15 | Windows CI (via MSVC / vcpkg) | `DONE` |
| 1.16 | macOS CI build coverage (runtime rendering fix tracked in 4.1) | `DONE` |

---

## Phase 2 - Testing Infrastructure

> *Goal: make it possible to validate game logic without running the full
> graphical application.*

| # | Task | Status |
|---|------|--------|
| 2.1 | Introduce **Google Test** (or Catch2) as a CMake `FetchContent` dependency | `DONE` |
| 2.2 | Headless / mock rendering mode so tests can run without a display | `DONE` |
| 2.3 | Unit tests for `base/math` (Vector3, Matrix3, Quaternion, bluntmath) | `DONE` |
| 2.4 | Unit tests for ball-physics calculations | `DONE` |
| 2.5 | Unit tests for player-velocity state machine | `DONE` |
| 2.6 | Integration test: simulate a full 90-second match and assert score/state | `DONE` |
| 2.7 | Run tests in GitHub Actions CI | `DONE` |

---

## Phase 3 - Gameplay Features

> *Goal: add commonly requested features and quality-of-life improvements.*

| # | Task | Status |
|---|------|--------|
| 3.1 | **Replay system** - record match state and play it back | `DONE` |
| 3.2 | **Statistics overlay** - shots on target, possession %, pass accuracy | `DONE` |
| 3.3 | **Save/Load match state** (SQLite-backed) | `DONE` |
| 3.4 | **Improved AI** - add pressure / counter-press tactics | `DONE` |
| 3.5 | **Set-piece editor** - configurable free-kick / corner routines | `DONE` |
| 3.6 | **Player stamina system** - fatigue affecting sprint speed | `DONE` |
| 3.7 | **Injury / substitution system** | `DONE` |
| 3.8 | **Weather effects** - rain / wind altering ball trajectory | `IDEA` |
| 3.9 | **Dynamic crowd audio** - crowd reacts to chances and goals | `DONE` |

---

## Phase 4 - Platform and UX

> *Goal: broaden platform support and improve the user experience.*

| # | Task | Status |
|---|------|--------|
| 4.1 | Fix macOS rendering (dispatch OpenGL calls to main thread) | `DONE` |
| 4.2 | Windows CI/CD with automated release artifacts | `DONE` |
| 4.3 | **HiDPI / Retina display** support | `DONE` |
| 4.4 | **Resolution selector** in the options menu | `DONE` |
| 4.5 | **Controller remapping** in-game menu | `DONE` |
| 4.6 | **Localization / i18n** scaffolding | `DONE` |
| 4.7 | **Shader improvements** - normal mapping, ambient occlusion | `DONE` |
| 4.8 | **Widescreen camera** preset (16:9 / 21:9) | `DONE` |
| 4.9 | **UI/Menu modernization** - dark sports theme, vertical main menu, centered HUD | `DONE` |

---

## Phase 5 - Developer Experience

| # | Task | Status |
|---|------|--------|
| 5.1 | Generate API docs with **Doxygen** | `PLANNED` |
| 5.2 | Architecture overview document | `PLANNED` |
| 5.3 | Docker-based development environment | `PLANNED` |
| 5.4 | Script to auto-fetch and set up data assets | `PLANNED` |

---

## Phase 6 - Advanced Gameplay Modes (myGM, myCoach, Career)

> *Goal: introduce deep management and career modes that give players
> long-term goals and immersion beyond single matches.*

| # | Task | Status |
|---|------|--------|
| 6.1 | **myCoach Mode** - Create-a-coach, skill trees, training drills focus | `PLANNED` |
| 6.2 | **myGM Mode** - Full general manager controls (trades, contracts, draft, budget) | `PLANNED` |
| 6.3 | **Player Career Mode** - Control a single player, request transfers, grow attributes | `PLANNED` |
| 6.4 | **Manager Career Mode** - Handle tactics, lineups, press conferences, board objectives | `PLANNED` |
| 6.5 | **Dynamic transfer market** - AI teams make bids, negotiate, loan deals | `PLANNED` |
| 6.6 | **Contract negotiation system** - salary, bonuses, release clauses, contract length | `PLANNED` |
| 6.7 | **Draft system** - annual rookie draft with scouting reports and pick trading | `PLANNED` |
| 6.8 | **Scouting network** - hire scouts, discover hidden gems, attribute uncertainty | `PLANNED` |
| 6.9 | **Youth academy** - promote youth players, develop prospects over seasons | `PLANNED` |
| 6.10 | **Staff management** - hire / fire assistant coaches, fitness coaches, medical staff | `PLANNED` |
| 6.11 | **Facility upgrades** - improve training grounds, stadium, medical center | `PLANNED` |
| 6.12 | **Morale and chemistry system** - team cohesion affects on-pitch performance | `PLANNED` |
| 6.13 | **Press conference / media interactions** - dialogue choices impact reputation | `IDEA` |
| 6.14 | **Achievement / milestone system** - personal and club records, hall of fame | `PLANNED` |
| 6.15 | **Multi-season persistence** - save career progress across multiple seasons | `PLANNED` |
| 6.16 | **League expansion / relegation** - dynamic league structure with promotion / demotion | `IDEA` |
| 6.17 | **Custom league creation** - users define teams, divisions, rules | `IDEA` |

---

## How to Contribute

Pick any `PLANNED` item, open an issue to discuss, then send a pull request.
See [CONTRIBUTING.md](CONTRIBUTING.md) for coding style, branch naming and
PR process.
