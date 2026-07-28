# AGENTS.md

## Cursor Cloud specific instructions

### Product
Single product: **League-Soccer / Gameplay Football**, a native C++17 SDL2 + OpenGL
football game producing one executable (`gameplayfootball`). No web/backend services;
team/league/career data lives in embedded SQLite files under `data/`. Standard
build/run/test commands are documented in `README.md` and `CONTRIBUTING.md`.

### Toolchain notes
- Dependencies are installed by the environment update script (CMake, Ninja, GCC,
  SDL2 stack, Boost, SQLite3, OpenGL/Mesa, OpenAL, Xvfb, clang-format/tidy).
- The default `c++`/`cc` alternatives are **clang**, which selects the GCC 14
  toolchain; `libstdc++-14-dev` must be present or clang fails with
  `cannot find -lstdc++`. It is installed by the update script. Both the default
  clang and `gcc`/`g++` build the code; CI uses GCC.

### Headless tests (works today)
This is the reliable buildable/testable surface. Use a fresh, gitignored build dir
(e.g. `build-test`) rather than the stale `build*` dirs committed in the repo:
```
cmake -S . -B build-test -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DGAMEPLAYFOOTBALL_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build build-test --parallel "$(nproc)"
ctest --test-dir build-test --output-on-failure
```
As of this setup, 81/84 tests pass. The 3 `LeagueBootstrapIntegrationTest` cases
(#47–49) abort at runtime due to a pre-existing logic bug in the league-bootstrap
code (they never linked before `leaguesetup.cpp` was wired into the build), not an
environment problem.

### Full game build (currently broken on `master`)
`GAMEPLAYFOOTBALL_BUILD_GAME=ON` (the default) does **not** compile on `master`:
there are ~170 compile errors from incomplete career-mode / scene-manager work,
concentrated in `src/menu/career/*` (ambiguous `CareerDatabase`, missing
`StaffMember`/`CustomLeagueConfig` members, undeclared page members) and
`src/managers/scenemanager.cpp`. This is unfinished feature code, not a setup issue.
Once it compiles, run headless with assets:
```
cmake -S . -B build-game -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-game --parallel "$(nproc)"
scripts/setup_assets.sh --build-dir build-game --symlink   # assets must sit next to the binary
xvfb-run -a ./build-game/gameplayfootball data/menu_smoke_quick_match.config
```

### Repo gotchas
- Many `build*/` directories (including WSL ones) are committed to the repo despite
  being in `.gitignore`. Do **not** `git add -A`; it will stage/delete thousands of
  build artifacts. Stage only the specific source files you change.
- CI (`.github/workflows/ci.yml`) only triggers on `main`, `copilot/**`, and
  `claude/**`, so `master` is not gated and can contain broken commits.
- Lint is `clang-format` (Google style); CI only checks changed files. Some existing
  files on `master` already contain formatting violations.
