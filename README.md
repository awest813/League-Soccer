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
   - [macOS](#macos-work-in-progress)
   - [Windows](#windows-work-in-progress)
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
- 🖥️ **Modern dark UI** with vertical main menu and centred HUD
- 🔁 **Continuous Integration** on every commit (Ubuntu/Linux)

See [ROADMAP.md](ROADMAP.md) for planned features including replay systems, career modes, and improved AI.

---

## Platform Support

| Platform | Builds | Runs | Notes |
|----------|--------|------|-------|
| Linux    | ✅     | ✅   | Fully supported |
| macOS    | ✅     | ⚠️   | Compiles; OpenGL main-thread fix pending ([#ROADMAP 4.1](ROADMAP.md)) |
| Windows  | ✅     | ✅   | Build via MSVC + vcpkg |

---

## Building from Source

### Linux

Install required dependencies:
```bash
sudo apt-get install git cmake build-essential libgl1-mesa-dev \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev \
  libopenal-dev \
  libboost-system-dev libboost-thread-dev libboost-filesystem-dev \
  libsqlite3-dev xvfb
```

Clone, configure, and build:
```bash
git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer

# Copy assets into the build directory
cp -R data/. build

cd build
cmake ..
make -j$(nproc)

./gameplayfootball
```

### macOS (Work in Progress)

> **Note**: The game compiles on macOS but does not yet run because OpenGL
> rendering must happen on the main thread.  This is tracked in
> [ROADMAP 4.1](ROADMAP.md).

Install [Homebrew](https://brew.sh/), then:

```bash
brew install git cmake sdl2 sdl2_image sdl2_ttf sdl2_gfx boost openal-soft

git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer
cp -R data/. build

cd build
cmake ..
make -j$(nproc)
```

### Windows (Work in Progress)

**Prerequisites:**
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) – *Desktop development with C++* workload
- [Git](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/) – add to `PATH`

**Set up vcpkg:**
```bat
cd C:\dev
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
```

**Install dependencies:**
```bat
.\vcpkg\vcpkg.exe install --triplet x86-windows ^
  boost:x86-windows sdl2 sdl2-image[libjpeg-turbo] ^
  sdl2-ttf sdl2-gfx opengl openal-soft sqlite3
```

**Clone and build:**
```bat
cd C:\dev
git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer

xcopy /e /i data build\Debug
xcopy /e /i data build\Release

cd build
cmake .. -DCMAKE_GENERATOR_PLATFORM=Win32 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE

cmake --build . --parallel --config Release
```

Run `build\Release\gameplayfootball.exe`.

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
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j$(nproc)
```

Code style is enforced by [`.clang-format`](.clang-format) (Google/C++17 style).  Format before committing:

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
├── CMakeLists.txt    # Build configuration
├── ROADMAP.md        # Planned improvements
└── CONTRIBUTING.md   # Contribution guide
```

---

## Roadmap & Contributing

- See **[ROADMAP.md](ROADMAP.md)** for the full list of planned features across six phases (modernisation, testing, gameplay, platform, developer experience, career modes).
- See **[CONTRIBUTING.md](CONTRIBUTING.md)** for coding style, branch naming, commit conventions, and PR guidelines.
- Found a bug or have an idea?  [Open an issue](../../issues).

PRs are always welcome — pick any 📋 item from the roadmap, open an issue to discuss, then send a pull request.

---

## Acknowledgements

- **[Bastiaan Konings Schuiling](http://www.properlydecent.com/)** – original author of GameplayFootball.  If you'd like to support his work, his Bitcoin address is `1JHnTe2QQj8RL281fXFiyvK9igj2VhPh2t`.
- **[Google Brain / Google Research Football](https://github.com/google-research/football)** – library modernisation and CMake improvements.
- All community contributors who have submitted fixes and enhancements.
