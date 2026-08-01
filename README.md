# League-Soccer

[![CI](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml/badge.svg)](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-Public_Domain-blue)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](CMakeLists.txt)

League-Soccer is a revived, open-source football (soccer) game project built around a modern C++17 engine. It started as a fork of the discontinued GameplayFootball project and carries forward the original game design while modernizing the build system, tooling, and cross-platform support.

The repository focuses on a playable match experience with AI opponents, menus, match simulation, audio, and data-driven game content. It is also structured in a way that makes it approachable for contributors who want to work on gameplay systems, rendering, UI, or engine internals.

## What you will find here

- A playable football game with multiple UI screens and match flow
- A modular C++ codebase organized around gameplay, rendering, input, menus, and data
- Cross-platform CMake builds for Linux, macOS, and Windows
- A testable architecture that supports headless unit and integration testing
- Developer-friendly tooling such as clang-format, Doxygen, and CI workflows

## Project highlights

- ⚽ Match simulation and AI-driven gameplay
- 🎮 Keyboard and gamepad input support
- 🏟️ 3D rendering with OpenGL and supporting graphics systems
- 🔊 Audio support via OpenAL (with a fallback path when unavailable)
- 🗄️ SQLite-backed data handling for teams, players, and match state
- 🧪 GoogleTest-based test infrastructure for core game logic

## Quick start

### Prerequisites

You will need:

- CMake 3.14 or newer
- A modern C++17 compiler
- SDL2, SDL2_image, SDL2_ttf, SDL2_gfx, Boost, SQLite3
- OpenGL and, optionally, OpenAL development packages

The repository provides helper scripts for common setups:

- `scripts/setup_linux_deps.sh`
- `scripts/setup_windows_deps.ps1`

### Linux

Install dependencies (Ubuntu/Debian example):

```bash
sudo apt-get update
sudo apt-get install -y git cmake build-essential \
  libgl1-mesa-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev \
  libsdl2-gfx-dev libopenal-dev libboost-dev libsqlite3-dev xvfb
```

Configure and build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Run the game from the build directory:

```bash
(cd build && ./gameplayfootball)
```

The build system copies runtime assets next to the executable automatically.

### macOS

Using Homebrew:

```bash
brew install git cmake sdl2 sdl2_image sdl2_ttf sdl2_gfx boost openal-soft sqlite
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
(cd build && ./gameplayfootball)
```

### Windows

Windows builds are supported through Visual Studio 2022 and vcpkg.

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build-win --parallel --config Release
```

Run the executable from the build output directory:

```powershell
build-win\Release\gameplayfootball.exe
```

### Docker

A Dockerfile and compose file are included for container-based development:

```bash
docker build -t gameplayfootball-dev .
# or
docker compose up
```

## Project layout

```text
League-Soccer/
├── src/                # Core C++ sources
│   ├── base/           # Math, geometry, logging, utilities
│   ├── framework/      # Engine framework and task scheduling
│   ├── managers/       # Global managers and service registries
│   ├── menu/           # Menus and UI systems
│   ├── onthepitch/     # Match simulation, players, AI, ball physics
│   ├── scene/          # Scene graph and object model
│   ├── systems/        # Rendering, audio, physics subsystems
│   └── utils/          # Shared helpers and data utilities
├── data/               # Assets, config, databases, locale, and media
├── tests/              # GoogleTest suites
├── scripts/            # Setup and packaging helpers
├── CMakeLists.txt      # Build configuration
└── ARCHITECTURE.md     # Higher-level engine overview
```

## Building for development

For a debug build with IDE-friendly compile commands:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
```

For headless tests (no SDL/OpenGL runtime required):

```bash
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DGAMEPLAYFOOTBALL_BUILD_GAME=OFF
cmake --build build-test --parallel
ctest --test-dir build-test --output-on-failure
```

## Controls

The game uses a keyboard-first control scheme, with gamepad support available as well.

### In-game

| Action | Key |
| --- | --- |
| Move player | Arrow keys / WASD |
| Sprint | Shift |
| Short pass | A |
| Long pass / cross | S |
| Shoot | D |
| Tackle / press | F |
| Switch player | Space |
| Pause | Escape |

### Menu navigation

| Action | Key |
| --- | --- |
| Navigate | Arrow keys |
| Confirm | Enter |
| Back | Escape |

## Documentation

Helpful project documents are included alongside the code:

- `ARCHITECTURE.md` – overview of the engine, subsystems, and design patterns
- `ROADMAP.md` – planned improvements and completed milestones
- `CONTRIBUTING.md` – contribution workflow, coding style, and PR expectations

## Contributing

Contributions are welcome. If you want to help:

1. Fork the repository and create a branch for your change.
2. Build and test locally.
3. Follow the existing C++17 and clang-format conventions.
4. Open a pull request with context for the change.

For formatting, run:

```bash
find src -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

## License and acknowledgements

This project is released under the Public Domain license. It is a modernized continuation of the original GameplayFootball work by Bastiaan Konings Schuiling and builds on contributions from the broader open-source community.

For more details, see:

- [LICENSE](LICENSE)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [ROADMAP.md](ROADMAP.md)
- [CONTRIBUTING.md](CONTRIBUTING.md)
