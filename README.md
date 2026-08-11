# League-Soccer (Gameplay Football)

[![CI](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml/badge.svg)](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](CMakeLists.txt)
[![CMake](https://img.shields.io/badge/CMake-3.14%2B-blue)](CMakeLists.txt)
[![License](https://img.shields.io/badge/License-Apache--2.0-blue)](LICENSE)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-black)](README.md)

**League-Soccer** is a fully featured, open-source football (soccer) game —
a 3D match engine, tactical AI, and deep management/career modes — rebuilt
from the archaeology of the discontinued
[GameplayFootball](https://github.com/BazkieBumpercar/GameplayFootball) project.

The engine's lineage runs through [Google Research Football](https://github.com/google-research/football)
(Google Brain's RL environment, which modernised the libraries but kept only
the physics core). This project merges those modern upstream improvements with
years of community contributions to restore and expand everything Google
stripped out — the menus, audio, full gameplay, and beyond.

> **tl;dr** — a modern, playable football game you can build on Linux, macOS,
> and Windows with one command, featuring full single matches plus a
> season-long career mode.

---

## Table of Contents

- [Highlights](#highlights)
- [Game Modes](#game-modes)
- [Features](#features)
- [Screenshots](#screenshots)
- [Platform Support](#platform-support)
- [Getting the Code](#getting-the-code)
- [Building & Running](#building--running)
  - [Linux](#linux)
  - [macOS](#macos)
  - [Windows](#windows)
  - [Docker](#docker)
  - [NixOS](#nixos)
- [Controls](#controls)
- [Configuration & Assets](#configuration--assets)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Development](#development)
- [Roadmap & Contributing](#roadmap--contributing)
- [Acknowledgements](#acknowledgements)
- [License](#license)

---

## Highlights

- **⚽ 3D match engine** — 22 players, custom ball physics (spin, bounce, wind),
  a tactical AI referee, and full-time match simulation with offside, fouls,
  set pieces, and statistics.
- **🎮 Keyboard + gamepad** — remappable controls for up to two players,
  with gamepad calibration and mapping.
- **🧠 Real football AI** — pressure and counter-press tactics, per-player
  roles, velocity/stamina state machine, and dynamic team AI controllers.
- **🛠️ Deep management modes** — run a club as Owner, GM, or Coach across
  transfer markets, contracts, drafts, scouting, youth academies, staff,
  facilities, and press conferences.
- **🔁 Replay system** — record and play back any match.
- **📊 Live statistics** — shots, possession, pass accuracy, and more.
- **🌍 5 languages** — English, Spanish, French, German, and Portuguese.
- **🗄️ SQLite persistence** — matches, leagues, and careers save across
  seasons.

---

## Game Modes

| Mode | What you do |
|------|-------------|
| **Quick Match** | Pick two teams and play a full match now. |
| **League** | Compete across a season — calendars, standings, inbox, transfers, contracts, and club management. |
| **Career (Manager)** | Choose a squad, handle tactics and lineups, negotiate transfers and contracts, manage finances, the board, and the press. |
| **Career (Owner)** | The full club vision — stadium, finances, staff hiring, sponsors, and the board room. |
| **myCoach / myGM / Player Career** | Build a coach, run the front office, or drive a single player toward stardom. |
| **Custom League** | Define your own teams, divisions, and rules; expand or contract the league structure. |

Career progress persists across seasons — promote youth talent from the
academy, scout hidden gems, trade in the transfer market and draft, negotiate
contracts, and watch team morale and chemistry shape on-pitch performance.

---

## Features

### On the pitch
- Full 90-minute match simulation with a playable match clock and stoppage time.
- Custom `BallPhysics` — trajectory, spin, bounce, and weather/wind effects.
- Per-player AI with sprint/stamina fatigue and substitutions/injuries.
- Referee and officials — offside lines, fouls, and set-piece routines
  (editable free-kick / corner editor).
- Replay and post-match highlights with full match history.

### The club & world
- SQLite-backed save system for matches, leagues, and multi-season careers.
- Transfer market with AI bidding, negotiation, loans, and free agency.
- Annual draft, scouting network, youth academy, and player development.
- Staff management and facility upgrades that affect performance.
- Dynamic league expansion / relegation and custom league creation.

### Presentation & tech
- Modern dark sports-themed UI with a vertical main menu and centered HUD.
- 3D stadium rendering via OpenGL shaders — normal mapping, ambient
  occlusion, and widescreen (16:9 / 21:9) camera presets.
- Spatial audio via OpenAL with a crowd that reacts to chances and goals.
- HiDPI / Retina support and an in-game resolution selector.
- Modular, testable C++17 codebase with target-based CMake.

---

## Screenshots

> Screenshots coming soon. In the meantime, run the game yourself — building
> takes only minutes (see [Building & Running](#building--running)).

---

## Platform Support

| Platform | Builds | Runs | Notes |
|----------|:------:|:----:|-------|
| **Linux** | ✅ | ✅ | Debian/Ubuntu/Mint, Fedora/RHEL, Arch/Manjaro, openSUSE, Void, Alpine, Gentoo, NixOS + Docker — one-command `./build.sh` |
| **macOS** | ✅ | ✅ | Homebrew deps; rendered & released by CI on macOS 14 |
| **Windows** | ✅ | ✅ | MSVC + vcpkg, with a runnable release artifact from CI |

CI builds and tests all three platforms on every push and pull request.

---

## Getting the Code

```bash
git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer
```

---

## Building & Running

### Linux

The quickest path is the two helper scripts — they install the right
dependencies for your distro, configure, and build:

```bash
./build.sh          # installs deps (auto-detects your package manager) + builds
./run.sh            # launches the game
```

`build.sh` options: `--release`, `--debug`, `--clean`, `--no-deps` (skip the
dependency step), `--jobs N`. Run `./build.sh --help` for the full list.
Dependencies are detected automatically (via `/etc/os-release`) for
**Debian/Ubuntu/Mint/Devuan**, **Fedora/RHEL**, **Arch/Manjaro**, **openSUSE**,
**Void**, **Alpine**, and **Gentoo**; on other distros see the
[manual steps](#proving-manual-build) below.

**NixOS** is declarative — use the ready-made dev-shell instead of installing
system packages:

```bash
nix develop      # or: nix-shell
./build.sh --no-deps
```

<a name="proving-manual-build"></a>
<details>
<summary>Manual build (advanced)</summary>

```bash
# Install dependencies yourself, or auto-detect your distro:
bash scripts/setup_linux_deps.sh          # also supports --minimal / --with-tools

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Assets (media/, databases/, locale/, football.config) are copied next to the
# binary by CMake POST_BUILD. For a manual/symlink install:
#   scripts/setup_assets.sh --build-dir build --symlink --force

(cd build && ./gameplayfootball)
```

</details>

### macOS

Install [Homebrew](https://brew.sh/), then:

```bash
brew install git cmake sdl2 sdl2_image sdl2_ttf sdl2_gfx boost openal-soft sqlite

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
(cd build && ./gameplayfootball)
```

### Windows

**Prerequisites (one-time):**
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) — *Desktop development with C++* workload
- [Git](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/) — add to `PATH`

The helper scripts mirror the Linux experience: they bootstrap
[vcpkg](https://github.com/microsoft/vcpkg), install the dependencies in
[`vcpkg.json`](vcpkg.json), then build and run:

```powershell
# First run only — if PowerShell refuses to run the scripts, allow for this
# session:
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

.\build.ps1          # bootstraps vcpkg + installs deps + builds (Release)
.\run.ps1            # launches the game
```

`build.ps1` options: `-DebugBuild`, `-Clean`, `-NoDeps`, `-Jobs N`,
`-VcpkgRoot <path>`, `-Triplet <name>`. Run `.\build.ps1 -Help` for the full
list. (_PowerShell reserves `-Debug` as a common parameter, so the debug build
flag is `-DebugBuild`._)

To assemble a distributable folder (exe + DLLs + assets):

```powershell
.\scripts\package_windows.ps1 -BuildDir build-win\Release -OutDir dist\windows-x64 `
  -VcpkgRoot C:\dev\vcpkg -Triplet x64-windows
```

### Docker

```bash
docker build -t gameplayfootball-dev .
# or: docker compose up
```

The image configures and builds a Debug tree under `/workspace/build` and runs
the test suite. See [`Dockerfile`](Dockerfile) and
[`docker-compose.yml`](docker-compose.yml).

### NixOS

The repository ships a [`shell.nix`](shell.nix) providing every dependency
(SDL2 family, OpenGL/Mesa, OpenAL, SQLite, Boost, Xvfb, clang-tools, doxygen,
graphviz). Enter it and build with `--no-deps`:

```bash
nix-shell          # or: nix develop
./build.sh --no-deps
```

---

## Controls

> Default keyboard layout. Gamepad support — including remapping and
> calibration — is configurable via the Settings menu.

### In-Game

| Action | Key |
|--------|-----|
| Move player | Arrow keys / **WASD** |
| Sprint | Hold **Shift** |
| Short pass | **A** |
| Long pass / Cross | **S** |
| Shoot | **D** |
| Tackle / Press | **F** |
| Switch player | **Space** |
| Pause | **Escape** |

### Gameplay Options

- Match duration is selectable in five-minute steps from 5 to 90 minutes. It
  measures active play time, so the clock stopping while the ball is out does
  not shorten the selected match.
- Human slow-dribble, run, and sprint speeds can be tuned independently under
  **Settings > Gameplay**. The factory values preserve the original movement.
- Player switching can be **Assisted** or **Fully manual**. In fully manual
  mode, control changes only when **Switch player** is pressed, even after an
  AI teammate wins or receives the ball. Press **Short pass** while controlling
  an off-ball teammate to call for a pass from the AI ball carrier.

### Menu Navigation

| Action | Key |
|--------|-----|
| Navigate | Arrow keys |
| Confirm | **Enter** |
| Back | **Escape** |

---

## Configuration & Assets

- **Save data** is stored via SQLite (matches, leagues, careers).
- **Language** can be changed in the Settings menu (English, Spanish, French,
  German, Portuguese).
- Assets live in [`data/`](data/) — SQLite databases, media, and locale files.
  They are copied next to the binary automatically at build time via CMake
  `POST_BUILD`; `scripts/setup_assets.sh` handles manual/symlink installs.

---

## Testing

The project includes five Google Test suites that run headless (no display, no
game window needed), so logic can be verified in CI and on any machine:

| Suite | Target | What it validates |
|-------|--------|-------------------|
| Math | `gameplayfootball_math_tests` | Vector3, Matrix3, Quaternion, bluntmath |
| Ball physics | `gameplayfootball_velocity_tests` | Player sprint/fatigue state machine |
| Match integration | `gameplayfootball_integration_tests` | Full 90-second headless match |
| League integration | `gameplayfootball_league_integration_tests` | League flow & persistence |
| Career | `gameplayfootball_career_tests` | Career mode logic |

Run them yourself:

```bash
cmake -S . -B build-test \
  -DGAMEPLAYFOOTBALL_BUILD_GAME=OFF \
  -DBUILD_TESTING=ON
cmake --build build-test --parallel
ctest --test-dir build-test --output-on-failure
```

Headless mode (`-DGAMEPLAYFOOTBALL_BUILD_GAME=OFF`) compiles only the modules
the tests need, so no SDL/OpenGL/OpenAL headers or display server are required.

---

## Project Structure

```
League-Soccer/
├── src/                   # C++17 source
│   ├── base/              # Math (Vector3, Matrix3, Quaternion), logging, utilities
│   ├── framework/         # Engine core: scheduler, task sequences, worker threads
│   ├── managers/          # High-level managers (scene, system, resource, task, event)
│   ├── scene/             # Scene graph — 2D/3D objects, resources
│   ├── systems/           # Rendering (OpenGL), audio (OpenAL), physics subsystems
│   ├── onthepitch/        # Match simulation, player AI, ball physics, referee
│   ├── menu/              # All UI screens + the GUI2 widget framework
│   ├── data/              # Game data structures (match, player, team, history)
│   ├── league/            # League code, SQLite queries, game defines
│   ├── hid/               # Keyboard + gamepad input abstractions
│   ├── loaders/           # Asset loaders (ASE models, images, audio)
│   ├── types/             # Base patterns (Observer, Singleton, RefCounted, …)
│   ├── utils/             # Shared utilities (localization, database, GUI)
│   └── misc/              # Standalone helpers
├── tests/                 # Google Test unit & integration suites
├── data/                  # Runtime assets (SQLite DB, media, locale)
├── scripts/               # Linux/Windows setup + packaging helpers
├── build.sh / run.sh      # One-shot Linux helpers
├── build.ps1 / run.ps1    # One-shot Windows helpers
├── shell.nix              # NixOS dev-shell
├── Dockerfile             # Containerized dev environment
├── vcpkg.json             # Windows dependency manifest
├── CMakeLists.txt         # Build configuration
├── sources.cmake          # Source-file list
└── ARCHITECTURE.md        # Deep-dive design doc
```

See **[ARCHITECTURE.md](ARCHITECTURE.md)** for a detailed design overview
(threading model, layer diagram, module descriptions, patterns).

---

## Development

Quick debug build with IDE/clangd support:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
```

Code style is enforced by [`.clang-format`](.clang-format) (Google / C++17).
Format before committing — CI fails on any file that doesn't match:

```bash
clang-format -i src/my_file.cpp                       # single file
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i   # all C++
```

`clang-tidy` runs on changed files in CI to catch additional issues.

**CMake options:**

| Option | Default | Effect |
|--------|---------|--------|
| `GAMEPLAYFOOTBALL_BUILD_GAME` | `ON` | Build the full SDL/OpenGL game binary |
| `BUILD_TESTING` | `OFF` | Build and register the Google Test suites |
| `GAMEPLAYFOOTBALL_BUILD_DOCS` | `OFF` | Add a `docs` CMake target that runs Doxygen |

---

## Roadmap & Contributing

Every roadmap phase is tracked in **[ROADMAP.md](ROADMAP.md)** — modernization,
testing infrastructure, gameplay, platform & UX, developer experience, and the
advanced career modes are all marked done, with new ideas welcome.

> **Note on GitHub Issues:** the Issues tab on GitHub is **not active** for
> this project, so bug reports and feature requests cannot be filed there.
> Contribution is pull-request based:
>
> - Found a bug or want a feature? **Send a pull request** — it is the most
>   reliable way to get something addressed. See
>   **[CONTRIBUTING.md](CONTRIBUTING.md)** for guidelines.
> - Fork the repo, create a feature branch from `master`, and open a PR against
>   `master`.

PRs are always welcome — pick any roadmap item or fix you'd like to tackle, then
send a pull request.

---

## Acknowledgements

- **[Bastiaan Konings Schuiling](http://www.properlydecent.com/)** — original
  author of GameplayFootball.
- **[Google Brain / Google Research Football](https://github.com/google-research/football)** —
  library modernization and CMake improvements.
- **All community contributors** who have submitted fixes, features, and
  improvements.

---

## License

This project is licensed under the **[Apache License 2.0](LICENSE)**.

*Original GameplayFootball code by Bastiaan Konings Schuiling; build on the
Google Research Football modernization and the contributions of the open-source
community.*
