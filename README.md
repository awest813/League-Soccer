## Gameplay Football

[![CI](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml/badge.svg)](https://github.com/awest813/League-Soccer/actions/workflows/ci.yml)

Football game, a fork of the discontinued [GameplayFootball](https://github.com/BazkieBumpercar/GameplayFootball) written by [Bastiaan Konings Schuiling](http://www.properlydecent.com/).

In 2019, Google Brain picked up the game and created a Reinforcement Learning environment – [Google Research Football](https://github.com/google-research/football).  They updated the libraries but removed everything (menus, audio, etc.) not needed for their task.

This repository combines those improvements and other community forks to produce a playable game that compiles and runs on as many platforms as possible.  See [ROADMAP.md](ROADMAP.md) for planned improvements.  PRs are always welcome – see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Building from source

### Linux

Install required dependencies:
```bash
sudo apt-get install git cmake build-essential libgl1-mesa-dev \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev \
  libopenal-dev \
  libboost-system-dev libboost-thread-dev libboost-filesystem-dev \
  libsqlite3-dev xvfb
```

Build and run:
```bash
# Clone the repository
git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer

# Copy assets into the build directory
cp -R data/. build

# Configure and compile
cd build
cmake ..
make -j$(nproc)

# Run
./gameplayfootball
```

### macOS (Work in Progress)

> **Note**: The game compiles on macOS but does not yet run because OpenGL
> rendering must happen on the main thread.  This is tracked in the
> [ROADMAP](ROADMAP.md).

Install [Homebrew](https://brew.sh/) then:

```bash
brew install git cmake sdl2 sdl2_image sdl2_ttf sdl2_gfx boost openal-soft

git clone https://github.com/awest813/League-Soccer.git
cd League-Soccer
cp -R data/. build

cd build
cmake ..
make -j$(nproc)

# Run (currently not working on macOS)
./gameplayfootball
```

### Windows (Work in Progress)

Download and install:
- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (with "Desktop development with C++" workload)
- [Git](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/) (add to PATH)

Install [vcpkg](https://github.com/microsoft/vcpkg):

```bat
cd C:\dev
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
```

Install required packages (all with the `x86-windows` triplet):

```bat
.\vcpkg\vcpkg.exe install --triplet x86-windows ^
  boost:x86-windows sdl2 sdl2-image[libjpeg-turbo] ^
  sdl2-ttf sdl2-gfx opengl openal-soft sqlite3
```

Clone and build:

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

## Developer Quick-Start

```bash
# Debug build with compile-commands for IDE/clangd support
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j$(nproc)
```

Code style is enforced by [`.clang-format`](.clang-format):

```bash
# Format a file
clang-format -i src/my_file.cpp
```

Run the unit tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DGAMEPLAYFOOTBALL_BUILD_GAME=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## Problems?

If you have any problems, please [open an issue](../../issues).

---

### Donate

If you want to thank Bastiaan for his original work, consider donating to his
Bitcoin address `1JHnTe2QQj8RL281fXFiyvK9igj2VhPh2t`.
