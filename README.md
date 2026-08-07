# League-Soccer (Gameplay Football)

[![CI](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml/badge.svg)](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-Public_Domain-blue)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](CMakeLists.txt)

A revived and actively developed football (soccer) game, forked from the discontinued [GameplayFootball](https://github.com/BazkieBumpercar/GameplayFootball) originally written by [Bastiaan Konings Schuiling](http://www.properlydecent.com/).

In 2019, Google Brain adopted the engine to build [Google Research Football](https://github.com/google-research/football), a Reinforcement Learning environment. They modernised the underlying libraries but stripped out everything not needed for RL (menus, audio, full gameplay, etc.).

This repository merges those upstream improvements with contributions from the broader community to produce **a fully playable game** that compiles and runs on as many platforms as possible.

---

## Table of Contents

1. [Features](#features)
2. [Platform Support](#platform-support)
3. [Building from Source](#building-from-source)
   - [Linux](#linux)
   - [macOS](#macos)
   - [Windows](#windows)
   - [Docker](#docker)
4. [Controls](#controls)
5. [Developer Quick-Start](#developer-quick-start)
6. [Project Structure](#project-structure)
7. [Roadmap & Contributing](#roadmap--contributing)
8. [Acknowledgements](#acknowledgements)

---

## Features

- ⚽ **Playable single-match football game** with AI opponents
- 🎮 **Keyboard & gamepad support**
- 🔊 **Spatial audio** via OpenAL
- 🏟️ **3D stadium rendering** with OpenGL shaders
- 🗄️ **SQLite-backed** team and player data
- 🧩 **Modular C++17 codebase** – easy to extend
- 🖥️ **Modern dark UI** with vertical main menu and centered HUD
- 🔁 **Continuous Integration** on Linux, Windows, and macOS

See [ROADMAP.md](ROADMAP.md) for planned features including replay systems, career modes, and improved AI.

---

## Platform Support

| Platform | Builds | Runs | Notes |
|----------|--------|------|-------|
| Linux    | ✅     | ✅   | Debian/Fedora/Arch/openSUSE + Docker; one-shot `./build.sh` |
| macOS    | ✅     | ✅   | Homebrew + CI on macOS 14 |
| Windows  | ✅     | ✅   | MSVC + vcpkg; runnable CI artifact |

---

## Building from Source

### Linux

The quickest way to get playing is the two helper scripts, which install the
right dependencies for your distro and then build + run the game:

```bash
git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer

./build.sh          # installs deps (apt/dnf/pacman/zypper) + builds the game
./run.sh            # launches it
```
On Windows the equivalent is `.\build.ps1` then `.\run.ps1` — see [Windows](#windows).

`build.sh` options: `--debug`, `--clean`, `--no-deps` (skip the dep install),
`--jobs N`. Run `./build.sh --help` for the full list. Dependencies are
detected automatically for **Debian/Ubuntu/Mint**, **Fedora/RHEL**,
**Arch/Manjaro**, and **openSUSE**; on other distros see the manual steps below.

<details>
<summary>Manual build (advanced)</summary>

```bash
# Install dependencies yourself, or:
bash scripts/setup_linux_deps.sh          # detects distro + package manager

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Runtime assets (media/, databases/, locale/, football.config) are copied
# next to the binary by CMake POST_BUILD. For a manual/symlink install:
#   scripts/setup_assets.sh --build-dir build --symlink --force

(cd build && ./gameplayfootball)
```

</details>

### macOS

Install [Homebrew](https://brew.sh/), then:

```bash
brew install git cmake sdl2 sdl2_image sdl2_ttf sdl2_gfx boost openal-soft sqlite

git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
(cd build && ./gameplayfootball)
```

### Windows

**Prerequisites (one-time):**
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) – *Desktop development with C++* workload
- [Git](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/) – add to `PATH`

The quickest way to get playing is the two helper scripts, which mirror the
Linux `build.sh` / `run.sh`. They bootstrap [vcpkg](https://github.com/microsoft/vcpkg),
install the dependencies declared in [`vcpkg.json`](vcpkg.json) (manifest mode),
then configure + build + run the game:

```powershell
git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer

# First run only — if PowerShell refuses to run the scripts, allow them for
# this terminal session:
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

.\build.ps1          # bootstraps vcpkg + installs deps + builds (Release)
.\run.ps1            # launches the game
```

`build.ps1` options: `-DebugBuild`, `-Clean`, `-NoDeps` (skip the vcpkg/deps
step), `-Jobs N`, `-VcpkgRoot <path>`, `-Triplet <name>`. Run `.\build.ps1 -Help`
for the full list. CMake copies assets and (on CMake ≥ 3.21) runtime DLLs beside
the executable, so `run.ps1` finds everything without `PATH` edits. (PowerShell
reserves the `-Debug` common parameter, so the debug build flag is `-DebugBuild`.)

To assemble a distributable folder (exe + DLLs + assets):

```powershell
.\scripts\package_windows.ps1 -BuildDir build-win\Release -OutDir dist\windows-x64 `
  -VcpkgRoot C:\dev\vcpkg -Triplet x64-windows
```

This mirrors the `build-windows` job in [CI](.github/workflows/ci.yml).

<details>
<summary>Manual build (advanced)</summary>

If you prefer to drive vcpkg and CMake by hand:

```powershell
# Set up vcpkg (or let scripts\setup_windows_deps.ps1 do it for you)
cd C:\dev
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat

cd C:\dev\League-Soccer

# Manifest mode installs vcpkg.json deps automatically via the toolchain file.
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build-win --parallel --config Release
.\build-win\Release\gameplayfootball.exe
```

Classic (non-manifest) vcpkg install is also supported:

```powershell
.\vcpkg\vcpkg.exe install --triplet x64-windows
```

</details>

### Docker

```bash
docker build -t gameplayfootball-dev .
# or: docker compose up
```

The image configures and builds a Debug tree under `/workspace/build`. See
[`Dockerfile`](Dockerfile) and [`docker-compose.yml`](docker-compose.yml).

---

## Controls

> Default keyboard layout.  Gamepad support is also available.

### In-Game

| Action | Key |
|--------|-----|
| Move player | Arrow keys / WASD |
| Sprint | Hold **Shift** |
| Short pass | **A** |
| Long pass / Cross | **S** |
| Shoot | **D** |
| Tackle / Press | **F** |
| Switch player | **Space** |
| Pause | **Escape** |

### Menu Navigation

| Action | Key |
|--------|-----|
| Navigate | Arrow keys |
| Confirm | **Enter** |
| Back | **Escape** |

---

## Developer Quick-Start

```bash
# Debug build with compile-commands for IDE/clangd support
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
```

Code style is enforced by [`.clang-format`](.clang-format) (Google/C++17 style). Format before committing:

```bash
# Format a single file
clang-format -i src/my_file.cpp

# Format all C++ sources
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

CI will fail if any committed file doesn't match the formatter.

---

## Project Structure

```
League-Soccer/
├── src/              # C++ source code
│   ├── base/         # Math, geometry, utilities
│   ├── framework/    # Engine core (scene graph, renderer)
│   ├── onthepitch/   # Match simulation & player AI
│   ├── menu/         # UI screens and menus
│   ├── systems/      # Audio, input, physics subsystems
│   └── managers/     # High-level game-state managers
├── data/             # Assets (textures, models, sounds, config)
├── scripts/          # Linux/Windows setup and packaging helpers
├── build.sh          # One-shot Linux build (deps + configure + compile)
├── run.sh            # Launch the built game with its assets
├── build.ps1         # One-shot Windows build (vcpkg + deps + configure + compile)
├── run.ps1           # Launch the built game (Windows)
├── vcpkg.json        # Windows dependency manifest
├── CMakeLists.txt    # Build configuration
├── ROADMAP.md        # Planned improvements
└── CONTRIBUTING.md   # Contribution guide
```

Run the unit tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DGAMEPLAYFOOTBALL_BUILD_GAME=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## Roadmap & Contributing

- See **[ROADMAP.md](ROADMAP.md)** for the full list of planned features across six phases (modernization, testing, gameplay, platform, developer experience, career modes).
- See **[CONTRIBUTING.md](CONTRIBUTING.md)** for coding style, branch naming, commit conventions, and PR guidelines.
- Found a bug or have an idea?  [Open an issue](../../issues).

PRs are always welcome — pick any 📋 item from the roadmap, open an issue to discuss, then send a pull request.

---

## Acknowledgements

- **[Bastiaan Konings Schuiling](http://www.properlydecent.com/)** – original author of GameplayFootball.  If you'd like to support his work, his Bitcoin address is `1JHnTe2QQj8RL281fXFiyvK9igj2VhPh2t`.
- **[Google Brain / Google Research Football](https://github.com/google-research/football)** – library modernisation and CMake improvements.
- All community contributors who have submitted fixes and enhancements.
