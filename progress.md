Original prompt: keep going until you can go to a main menu click quick match and play an actuial match againt a cpu

- Investigated the start-match flow in code: `MainMenuPage -> ControllerSelectPage -> TeamSelectPage -> MatchOptionsPage -> LoadingMatchPage -> GamePage -> Match`.
- Confirmed the playable executable is not present in the current Windows workspace and the existing `build-headless` artifacts are Linux test binaries only.
- Confirmed a fresh WSL game configure currently fails locally because required packages such as Boost dev libraries are missing and cannot be installed non-interactively from this session.
- Found a likely UX blocker for dev/testing: `MenuTask::QuickStart()` auto-skips the main menu in non-release builds during the first 10 seconds.
- Updated the main menu label from `Play Match` to `Quick Match`.
- Changed `MenuTask::QuickStart()` so debug builds only auto-start a match when `quick_start` is explicitly set in config; otherwise they stay on the normal menu flow.
- Tried running `ctest` in `build-headless`, but the cached test files reference old CI paths under `/home/runner/work/...` and fail before executing tests on this machine.
- Remaining blocker for full end-to-end verification: build the real game on a machine with the SDL/OpenGL/OpenAL/Boost dependencies available, copy/link `data/` into that build directory, then manually verify `Intro/Main Menu -> Quick Match -> CPU team select -> Start match -> live gameplay`.
- Hardened menu/controller stability for missing or hot-unplugged devices:
  `ControllerSelectPage` now handles zero controllers with an on-screen notice + back button, skips unsafe keyboard/gamepad casts, and no longer asserts when controller counts change in-game.
- Hardened settings subpages against unavailable devices/data:
  keyboard save now checks for a real keyboard controller before writing config; gamepad list/setup/calibration/mapping/function pages now show a safe fallback page instead of indexing stale controller ids; missing-language and missing-resolution situations now fall back to a usable menu state.
- Hardened `SetActiveController()` in `MenuTask` so stale controller ids from saved side-selection state no longer index past the current controller list.
- Verification attempt on Windows:
  `cmake -S . -B build-menu-check -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` fails during configure because Boost headers are not installed locally (`Could NOT find Boost (missing: Boost_INCLUDE_DIR)`).
- Verification attempt on existing Linux-style artifacts:
  `ctest --test-dir build-headless --output-on-failure` still fails immediately because the cached generated test includes point at CI paths under `/home/runner/work/...`.

Build recovery work:

- Bootstrapped a local `vcpkg/` toolchain in the repo and installed the missing Windows dependencies needed for a real build:
  Boost components (`filesystem`, `signals2`, `system`, `thread`), `SDL2`, `SDL2_image`, `SDL2_ttf`, `SDL2_gfx`, `OpenGL`, `OpenAL Soft`, and `SQLite3`.
- Updated `CMakeLists.txt` so Windows configure/linking is more resilient:
  Boost discovery now works with both config and classic package layouts, imported SDL/OpenGL/OpenAL targets are chosen dynamically, and the game executable now builds as a normal `main`-based app instead of forcing `SDL2main`/`WIN32` entrypoint glue.
- Fixed multiple Windows/MSVC portability issues across the codebase:
  added `NOMINMAX` / `WIN32_LEAN_AND_MEAN` guards, replaced several GCC-only variable-length stack arrays with `std::vector`, switched the renderer to SDL OpenGL headers, and converted non-standard GNU compound-literal `Stat` initializers in `src/utils.cpp` to standard C++ brace construction.
- Fresh Windows configure now succeeds with:
  `cmake -S . -B build-win -G "Visual Studio 18 2026" -A x64 -DCMAKE_TOOLCHAIN_FILE=".../vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows`
- Fresh Windows build now succeeds with:
  `cmake --build build-win --config Debug --parallel`
  producing `build-win/Debug/gameplayfootball.exe`.
- Runtime smoke check now succeeds:
  launched `build-win/Debug/gameplayfootball.exe`, observed it stay alive for 10 seconds, then stopped it manually.
- Automated verification now succeeds on the recovered Windows build:
  `ctest --test-dir build-win -C Debug --output-on-failure`
  passed all 63 tests.

Warning cleanup work:

- Reduced a large batch of Windows narrowing/deprecation noise in shared code paths:
  `aabb.cpp`, `bluntmath.cpp`, `quaternion.cpp`, `properties.cpp`, `sdl_surface.cpp`, `base/utils.cpp`,
  `opengl_renderer3d.cpp`, `proceduralpitch.cpp`, `humanoidbase.cpp`, `officials.cpp`, and `src/utils.cpp`.
- Removed the remaining logged deprecation warnings in this pass by replacing unsafe CRT calls and format issues:
  `sprintf`/`fopen`/`strcpy` call sites were already migrated to safe equivalents, and the last observed `%lu` vs `size_t`
  mismatch in `src/utils.cpp` was updated to `%zu`.
- Eliminated a high-volume set of implicit double-to-float conversions by:
  replacing many unsuffixed numeric literals with `f` literals, explicitly casting `size_t`/integer values where intent is
  clear, and tightening a few math helpers to stay in `real`/`float` space instead of promoting to `double`.
- A logged warning-count snapshot taken during the cleanup dropped from the earlier pass totals down to:
  `C4244: 173`, `C4267: 24`, `C4305: 24`, `C4996: 0`, `C4477: 0`, `C4005: 2`.
- A transient MSBuild `user-mapped section open` / tlog issue briefly blocked one logged rebuild, but deleting the affected
  generated `gamelib.tlog` and `menulib.tlog` directories restored normal incremental builds.
- Verification after the warning pass:
  `cmake --build build-win --config Debug --target gameplayfootball --parallel 1`
  succeeds and still produces `build-win/Debug/gameplayfootball.exe`.

Weather effects (roadmap 3.8):

- Implemented wind + rain affecting ball trajectory in the tested physics core (`src/onthepitch/ballphysics.*`):
  `BallPhysicsConfig` gained a `wind` acceleration vector and a `[0,1]` `wetness` factor (defaults zero, so a
  calm/dry pitch is byte-identical to the old behavior). Wind is applied to the airborne ball scaled by an
  airborne factor (`1 - grassInfluenceBias`) so a grounded/shielded ball is unaffected; wetness scales ground
  friction down by up to 50% so the ball skids and keeps pace on a wet pitch.
- Plumbed weather into `Ball` (`src/onthepitch/ball.*`): reads `match_wind_x` / `match_wind_y` / `match_wetness`
  from configuration on construction, added a `SetWeather(wind, wetness)` setter plus getters, and feeds the
  values into `CalculatePrediction`'s physics config.
- Added regression + feature unit tests (`tests/onthepitch/ball_physics_test.cpp`): wind bends an airborne ball,
  wind does not shove a grounded ball, and a wet pitch lets the ball skid further. Full math/physics suite passes
  (17/17) via a `-DGAMEPLAYFOOTBALL_BUILD_GAME=OFF` tests-only build.
- Marked ROADMAP item 3.8 as DONE.

Match startup and gameplay recovery:

- Diagnosed match crash (0xC0000005 access violation):
  When a match began, `Player::Put2D()` attempted to draw an overhead triangle cursor using `GetDebugOverlay()->DrawTriangle()` and `GetDebugOverlay()->DrawLine()`. `debugOverlay` was only initialized when `GetDebugMode() == e_DebugMode_AI`, which is disabled in standard gameplay, leaving the intrusive pointer null. Calling `DrawTriangle()` through the null pointer crashed in `Image2D::subjectMutex.lock()` as soon as a player gained ball possession.
- Fixed `Player::Put2D()` in `src/onthepitch/player/player.cpp` by adding a null check on `GetDebugOverlay()` before drawing cursor primitives.
- Added a structured crash handler in `src/main.cpp` using `SetUnhandledExceptionFilter` + `DbgHelp` (`StackWalk64`, `SymFromAddr`, `SymGetLineFromAddr64`, `MiniDumpWriteDump`) to immediately capture call stacks and write minidumps if an unhandled exception occurs.
- Fixed `run.ps1` and `run.bat` parameter forwarding so `--` and arbitrary game args pass cleanly without PowerShell ambiguous parameter errors.
- Verified matches running end-to-end:
  - Quick Match smoke test (`menu_smoke_quick_match.config`): passed all markers and exited cleanly.
  - Full Match smoke test (`menu_smoke_full_match.config`): ran complete regulation and extra time match (Arsenal 0 - 0 Man Utd) and exited cleanly.
  - Gamepad match test (`menu_smoke_gamepad_match.config`): drove human player in live match without issues.
  - Full automated unit tests (`ctest`): 165/165 tests passed (100%).
  - Both Release and Debug binaries (`build-win/Release/gameplayfootball.exe` and `build-win/Debug/gameplayfootball.exe`) built and verified.

Career Mode Audit and Polish:

- 3D Matchday Integration & Exhibition Isolation:
  - Added `CareerPendingFixture` struct and tracking APIs (`SetPendingFixture`, `HasPendingFixture`, `ClearPendingFixture`, `ConsumePlayedFixture`) in `CareerDatabase`.
  - Fixed `GameOverPage` so completed exhibition/quick matches do not overwrite active career saves when returning to menus.
  - Fixed controller side assignment in `careerpages.cpp` (`PlayMatchFixture`): correctly assigns user controller to home side (`-1`) or away side (`1`) based on fixture orientation.
  - Fixed away score inversion: `ConsumePlayedFixture` correctly handles user goals whether playing at home or away.
- Multi-Slot Career Persistence UI:
  - Added `CareerSavePage` (`src/menu/career/career_save_page.hpp` & `.cpp`) supporting 5 distinct career save slots with save/load/delete buttons and metadata summaries (club name, persona mode, season, week, trust %, transfer budget, and last saved timestamp).
  - Integrated `CareerSavePage` into `CareerMenuPage` ("Load Saved Career"), `CareerHubPage`, and `OwnerHubPage` ("Save & Load Career").
  - Added `DeleteCareerSlot()` to `CareerDatabase` and centralized currency formatting (`FormatCareerMoney`) in `CareerCommon`.
- Localization:
  - Added full translation strings for Career Save/Load UI and hub navigation across `en.ini`, `de.ini`, `es.ini`, `fr.ini`, and `pt.ini`.
- Verification & Test Coverage:
  - Created `tests/career/career_fixture_test.cpp` testing pending fixture lifecycle, home/away score attribution, non-fixture safety, and slot save/load/delete operations.
  - Ran all 61 career tests in `gameplayfootball_career_tests.exe`: 61/61 passed (100%).
  - Ran career mode smoke test (`data/menu_smoke_career.config`): reached career menu and cleanly exited.
  - Re-verified Quick Match smoke test (`data/menu_smoke_quick_match.config`): passed end-to-end.

Career Mode & Seasons Overhaul:

- Dynamic League Standings & Leaderboards:
  - Created `CareerStandingsPage` (`src/menu/career/career_standings_page.hpp` & `.cpp`) registered as `e_PageID_CareerStandings`.
  - Implemented 20-team league standings table with rank, club, P, W, D, L, GF, GA, GD, points, and 5-match form guide.
  - Color-coded rows for User Club, Title Champions, Continental Qualification spots, and Relegation Zone.
  - Implemented Golden Boot Race / Top Scorers sidebar integrating user player goals and simulated star player stats.
- Season Simulation & Rollover Engine:
  - Implemented `CareerSim::GenerateLeagueStandings` and `CareerSim::GetTopScorers` deterministic simulation engine in `career_sim.cpp`.
  - Added `CareerSim::CalculateSeasonPrizeMoney`: position-based rewards ranging from €35,000,000 (Champions) down to €3,000,000, deposited directly to transfer budget upon advancing the season.
  - Enhanced `CareerSim::AdvanceSeason`: records championship titles into `legacyStats["titles"]`, adds trophy and Continental events, adjusts board trust, and processes contract expirations.
  - Enforced player contract expirations: players with 0 remaining years depart to free agency if squad size > 14, or receive an emergency 1-year wage-adjusted extension to prevent squads from dropping below minimum playable limits.
- Season Fixture Persistence:
  - Serialized and deserialized `save.season.fixtures` in `career_persistence.cpp` (`fixture.<i>=...`).
  - Automatically logged simulated and 3D match fixtures into `save.season.fixtures`.
- Added "🏆 League Table & Standings" button to `CareerHubPage`, `OwnerHubPage`, and `CareerSeasonPage`.
  - Renamed season button to "📅 Season Review & Rollover" on hubs.
  - Updated `CareerSeasonPage` to display prize money expectations and warnings for expiring player contracts.
- Localization:
  - Added all standings, top scorers, prize money, and contract warning strings across `en.ini`, `de.ini`, `es.ini`, `fr.ini`, and `pt.ini`.
- Verification:
  - Added 4 unit tests in `tests/career/career_fixture_test.cpp` covering standings generation, prize money and title awards, contract expiration and safety guards, and fixture persistence across save/load.
  - All 65 career unit tests passed (65/65, 100%) in `gameplayfootball_career_tests.exe`.
  - Full game binary `gameplayfootball.exe` compiled cleanly and passed runtime smoke test with zero errors.

Career & League Mode Load/Save Audit and Polish:

- Persistence Robustness & Backup Recovery:
  - Atomic writing via temporary files (`<path>.tmp`) with automatic `.bak` snapshotting on successful write.
  - Corrupt-file recovery: automatic fallback to `.bak` if the primary save file is missing or corrupted, followed by self-healing re-save.
  - Slot deletion cleans up `.save`, `.bak`, and `.tmp` files completely.
- Auto-Save Coverage:
  - Automatic background auto-saving at career creation, after matchday simulation, post-3D match consumption, season rollover, and when exiting career hubs.
  - Dedicated "Restore Auto-Save" feature on `CareerSavePage` with confirmation modal.
- Confirmation Modals & Safety Guards:
  - Added modal confirmation dialogs for overwriting occupied save slots, loading another slot over unsaved progress, and deleting slots.
- League Mode Save/Load Polish:
  - Disabled load/delete buttons on empty save slots.
  - Enhanced occupied slot metadata with manager name and creation timestamp.
- Localization & Testing:
  - Added full translation keys across all 5 languages (`en`, `de`, `es`, `fr`, `pt`).
  - Added persistence audit unit tests in `tests/career/career_fixture_test.cpp` (all 68 career tests passing).

Gameplay Sliders and UX Audit & Polish:

- Core Gui2Slider Widget Fixes & QoL:
  - Fixed critical bug in `ProcessWindowingEvent` where `quantizedValue` was never recomputed on keyboard/gamepad navigation, which had previously caused sliders to return stale values or feel unresponsive.
  - Improved change detection: click sound, redraws, and `sig_OnChange` are now triggered strictly when `quantizedValue` actually changes, eliminating click spam against min/max boundaries.
  - Added Activate-to-Default shortcut: pressing Enter, Space, or Gamepad A on any focused slider restores its value to the factory default helper value.
- Gameplay Settings UX & Readouts:
  - Formatted all 7 passing and shooting assistance sliders into intuitive readable labels (`Manual (0%)`, `Semi (X%)`, `Assisted (X%)`, `Full (X%)`).
  - Enhanced Agility and Acceleration sliders to display both percentage and actual physics multiplier (`50% (1.00x)` default).
  - Localized Quantization slider presets (`Full Analog`, `PES 16-way`, `PES 8-way`).
- Centralized Difficulty Mapping & Localization:
  - Created `src/utils/difficulty.hpp` providing shared `DifficultyToStep`, `DifficultyFromStep`, and `GetDifficultyName`.
  - Replaced hardcoded English difficulty names in Match Options, League Settings, and New League setup with localized strings (`Beginner`, `Amateur`, `Regular`, `Professional`, `Top Player`).
  - Updated Camera Menu to use localized factory default strings.
- Localization & Testing:
  - Added 12 new localization keys across all 5 languages (`en`, `de`, `es`, `fr`, `pt`).
  - Added 11 unit tests in `tests/menu/slider_ux_test.cpp` covering quantization rounding, boundary clamping, Activate-to-default, difficulty mapping, and physics multipliers (all 79 career tests passing).
  - Verified with gameplay settings, quick match, and league system smoke tests (all exit code 0).
