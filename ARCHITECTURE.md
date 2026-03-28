# Architecture Overview – Gameplay Football

> This document describes the high-level design of the **League-Soccer /
> Gameplay Football** project for new contributors and maintainers.
> See [ROADMAP.md](ROADMAP.md) for feature planning and
> [CONTRIBUTING.md](CONTRIBUTING.md) for coding conventions.

---

## Table of Contents

1. [Repository layout](#repository-layout)
2. [Layer diagram](#layer-diagram)
3. [Module descriptions](#module-descriptions)
   - [Entry point](#entry-point)
   - [Framework](#framework)
   - [Managers](#managers)
   - [Scene / object graph](#scene--object-graph)
   - [Systems – Graphics, Audio, Physics](#systems--graphics-audio-physics)
   - [On-the-pitch – match engine](#on-the-pitch--match-engine)
   - [Menu / UI](#menu--ui)
   - [Data layer](#data-layer)
   - [Human-input devices (HID)](#human-input-devices-hid)
   - [Base / utilities](#base--utilities)
4. [Key design patterns](#key-design-patterns)
5. [Threading model](#threading-model)
6. [Build system](#build-system)
7. [Data assets](#data-assets)
8. [Testing strategy](#testing-strategy)

---

## Repository layout

```
League-Soccer/
├── src/
│   ├── main.cpp / main.hpp    # Application entry point
│   ├── base/                  # Math (Vector3, Matrix3, Quaternion), logging, utilities
│   ├── framework/             # Scheduler, task sequences, worker threads
│   ├── managers/              # Scene, system, resource, task and event managers
│   ├── scene/                 # Scene graph – objects, resources, 2D/3D scenes
│   ├── systems/               # Rendering (OpenGL), audio (OpenAL), physics subsystems
│   ├── onthepitch/            # Match simulation, player AI, ball physics, referee
│   ├── menu/                  # All UI screens and the GUI2 widget framework
│   ├── data/                  # Game data structures (match, player, team, history)
│   ├── league/                # League code, database queries, game defines
│   ├── hid/                   # Keyboard and gamepad input abstractions
│   ├── loaders/               # Asset loaders (ASE models, images, audio)
│   ├── types/                 # Base patterns (Observer, Singleton, RefCounted, …)
│   ├── utils/                 # Shared utilities (animation, localization, database, GUI)
│   └── misc/                  # Standalone helpers (Perlin noise)
├── tests/                     # Google Test unit and integration tests
├── data/                      # Runtime assets (SQLite DB, media, locale files)
├── CMakeLists.txt             # Top-level CMake build
├── Doxyfile                   # Doxygen configuration
├── Dockerfile                 # Container-based development environment
├── docker-compose.yml         # Compose file for the dev container
└── scripts/
    └── setup_assets.sh        # Asset-setup helper script
```

---

## Layer diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│  Application Entry Point  (src/main.cpp)                             │
│  SDL2 window · OpenGL context · system/manager bootstrap             │
└─────────────────────────────┬────────────────────────────────────────┘
                              │
┌─────────────────────────────▼────────────────────────────────────────┐
│  Framework  (src/framework/)                                         │
│  Scheduler · TaskSequence · WorkerThread pool                        │
└────┬─────────────────────────────────────────────┬───────────────────┘
     │                                             │
┌────▼───────────────────┐             ┌───────────▼────────────────┐
│  Managers              │             │  Human-Input Devices        │
│  SceneManager          │             │  (src/hid/)                 │
│  SystemManager         │             │  Keyboard · Gamepad         │
│  ResourceManagerPool   │             └────────────────────────────┘
│  TaskManager           │
│  UserEventManager      │
└────┬───────────────────┘
     │
┌────▼───────────────────────────────────────────────────────────────┐
│  Scene / Object Graph  (src/scene/)                                │
│  Scene2D · Scene3D · Node · Object · ObjectFactory                 │
│  Resources: GeometryData · SoundBuffer · Texture                   │
└────┬──────────────────────────────────────────────────────────────┘
     │
┌────▼──────────────────────────────────────────────────────────────┐
│  Systems  (src/systems/)                                          │
│  GraphicsSystem (OpenGL) · AudioSystem (OpenAL) · PhysicsSystem   │
│  Each system owns a renderer, a scene proxy and object proxies.   │
└────┬──────────────────────────────────────────────────────────────┘
     │
     ├─────────────────────────────────────────┐
     │                                         │
┌────▼──────────────────┐         ┌────────────▼────────────────────┐
│  On-the-Pitch         │         │  Menu / UI                      │
│  (src/onthepitch/)    │         │  (src/menu/)                    │
│  Match · Team · Ball  │         │  MenuTask · PageFactory         │
│  Player + AI          │         │  Screens: MainMenu, Settings,   │
│  Referee · Officials  │         │  InGame, TeamSelect, …          │
│  BallPhysics          │         │  GUI2 widget framework          │
└────┬──────────────────┘         └─────────────────────────────────┘
     │
┌────▼──────────────────────────────────────────────────────────────┐
│  Data Layer  (src/data/, src/league/)                             │
│  MatchData · TeamData · PlayerData · MatchHistory                 │
│  SQLite-backed persistence via Database / DbQuery helpers         │
└───────────────────────────────────────────────────────────────────┘
```

---

## Module descriptions

### Entry point

`src/main.cpp` bootstraps the entire application:

1. Initialises SDL2, creates a window, and sets up an OpenGL context.
2. Instantiates the **GraphicsSystem** and **AudioSystem**.
3. Creates the shared **Scene2D** and **Scene3D** scene graphs.
4. Creates the **Scheduler** and wires two `TaskSequence` pipelines:
   - `graphicsSequence` – render loop.
   - `gameSequence` – game-logic update loop.
5. Instantiates `GameTask` (match engine) and `MenuTask` (UI) and hands
   control to the scheduler.
6. On exit, cleanly tears down all systems and managers in reverse order.

---

### Framework

`src/framework/` provides the task-scheduling backbone.

| Class | Responsibility |
|---|---|
| `Scheduler` | Owns the worker-thread pool; drives `TaskSequence` execution each tick. |
| `TaskSequence` | An ordered list of `IUserTask` steps executed by the scheduler. |
| `WorkerThread` | A `std::thread` wrapper with a message queue for safe inter-thread communication. |

All game systems register themselves as tasks with the scheduler, decoupling execution order from object ownership.

---

### Managers

`src/managers/` provides global service registries.

| Manager | Responsibility |
|---|---|
| `SceneManager` | Registry for all active `IScene` instances. |
| `SystemManager` | Registry for all `ISystem` instances (Graphics, Audio, Physics). |
| `ResourceManagerPool` | Manages named `ResourceManager<T>` caches for textures, geometry, sounds, etc. |
| `TaskManager` | Associates `IUserTask` objects with their owning sequences. |
| `UserEventManager` | Routes SDL user events (custom event IDs) to registered handlers. |
| `EnvironmentManager` | Stores global game settings (resolution, fullscreen, locale, …). |

---

### Scene / object graph

`src/scene/` is the spatial scene representation, shared by all systems.

- **`IScene`** – abstract scene base (2D or 3D).
- **`Scene2D` / `Scene3D`** – concrete scene implementations holding node trees.
- **`Object`** – a scene node that can carry a `Spatial` transform and attach to systems.
- **`ObjectFactory`** – central factory for creating typed `Object` subclasses (Camera, Light, Geometry, Sound, …).
- **Resources** – `GeometryData`, `SoundBuffer`, `Texture` etc. are reference-counted via `RefCounted`; managed by `ResourceManager<T>`.

Each `Object` is registered with the relevant system's scene proxy when added to a scene, keeping the scene graph decoupled from renderer internals.

---

### Systems – Graphics, Audio, Physics

Each system follows the same layered interface pattern:

```
ISystem            ← abstract system interface
  ISystemTask      ← tick/update hook used by the scheduler
  ISystemScene     ← proxy for one scene's objects
  ISystemObject    ← proxy for one object's system-specific state
```

**GraphicsSystem** (`src/systems/graphics/`)

- Uses **OpenGL** via `OpenGLRenderer3D`.
- Manages shader programs, vertex buffers, and texture uploads.
- Supports normal mapping, ambient occlusion, and widescreen camera presets.
- On macOS all OpenGL calls are dispatched to the main thread.
- `Graphics_Overlay2D` handles the 2D HUD layer rendered on top of the 3D scene.

**AudioSystem** (`src/systems/audio/`)

- Uses **OpenAL** via `OpenALRenderer`.
- Spatially positions sounds in 3D; crowd audio dynamically reacts to match events.
- Sound resources (`SoundBuffer`) are loaded by `WavLoader` and cached.

**PhysicsSystem** (`src/systems/physics/`)

- Wraps **ODE** (Open Dynamics Engine) for rigid-body collision.
- Used primarily for stadium/environment geometry; in-game ball physics are handled by the custom `BallPhysics` module in `src/onthepitch/`.

---

### On-the-pitch – match engine

`src/onthepitch/` is the core simulation layer.

```
Match
  ├── Team  (×2)
  │     └── Player  (×11+)
  │           ├── HumanoidBase  – 3D skeletal representation
  │           ├── IController   – HumanController | ElizaController (AI)
  │           └── VelocityState – stamina/sprint state machine
  ├── Ball
  │     └── BallPhysics  – trajectory, spin, bounce, wind
  ├── Referee
  │     └── Officials
  └── TeamAIController  (×2)  – tactics, formation, pressing
```

**Key flows:**

- `Match::Update()` is called every game tick via `gameSequence`.
- Each `Player` polls its `IController` for an action (move, pass, shoot, tackle).
- `TeamAIController` runs pressure/counter-press logic and assigns roles.
- `BallPhysics` integrates velocity/acceleration each tick and detects goal events.
- `Referee` checks offside lines, fouls, and match-clock transitions (half-time, full-time).
- The **Replay** subsystem serialises `MatchState` snapshots each tick for post-match playback.

---

### Menu / UI

`src/menu/` owns all user-facing screens and the **GUI2** widget framework.

- `MenuTask` registers with the scheduler and calls `PageFactory` to instantiate the active page each frame.
- **`PageFactory`** maps `e_PageID` enumerators to concrete `Page` subclasses.
- Pages extend `gui2::Page` and lay out `gui2` widgets (Button, Text, Grid, Slider, …).
- `Localization` (singleton, `src/utils/localization.hpp`) loads locale strings from `data/locale/<lang>.ini` and provides `L("key")` translation calls throughout the UI.

Screens include: `MainMenu`, `TeamSelect`, `LoadingMatch`, `InGame` (HUD, radar, stats overlay), `GameOver`, `Settings` (graphics, audio, controls, language), `ReplayMenu`, `SetPieceEditor`, `MatchHistory`, and `LeagueScreens`.

---

### Data layer

`src/data/` defines the in-memory game data structures:

| Type | Contents |
|---|---|
| `PlayerData` | Name, attributes (pace, skill, stamina, …), current fatigue and injury state. |
| `TeamData` | Roster, formation, kit colours, set-piece configurations. |
| `MatchData` | Live score, match clock, statistics (shots, possession, pass accuracy). |
| `MatchHistory` | Serialised match events; loaded/saved via SQLite. |
| `SetPieceConfig` | Encoded corner / free-kick routines. |

Persistence is handled by `src/utils/database.hpp` (a thin wrapper around **SQLite3**) and the query helpers in `src/league/dbquery.hpp`.

---

### Human-input devices (HID)

`src/hid/` abstracts physical input:

- `IHIDevice` – common interface for readable device state.
- `Keyboard` – maps SDL keycodes; supports player 1 / player 2 key bindings.
- `Gamepad` – wraps SDL2 game-controller API; supports axis deadzones and button remapping stored in `EnvironmentManager`.

---

### Base / utilities

`src/base/` provides math primitives:

- `Vector3`, `Matrix3`, `Matrix4`, `Quaternion` – spatial maths.
- `bluntmath.hpp` – angle normalisation, lerp, clamp and other helpers.
- `AABB`, `Triangle`, `Plane`, `Line` – geometry primitives.
- `Log`, `Exception` – logging and error propagation.

`src/utils/` provides shared runtime utilities:

| Utility | Purpose |
|---|---|
| `Localization` | `.ini`-based i18n with `L("key")` helper. |
| `Database` | SQLite3 RAII wrapper. |
| `Animation` | Keyframe animation data and blending. |
| `ObjectLoader` | Loads `.object` asset bundles (geometry + material). |
| `Text2D` | SDL_ttf-based on-screen text rendering. |

---

## Key design patterns

| Pattern | Where used |
|---|---|
| **Observer / Subject** | `src/types/observer.hpp`, `subject.hpp` – event notifications between game objects (e.g. ball enters goal notifies referee). |
| **Singleton** | `EnvironmentManager`, `Localization`, manager singletons accessible via `GetXxx()` free functions. |
| **Template resource cache** | `ResourceManager<T>` keyed by filename string; reference-counted via `boost::intrusive_ptr`. |
| **Interface / strategy** | `IController` lets AI and human input share the same player-update path. `ISystem` / `ISystemScene` / `ISystemObject` decouple engine layers from implementations. |
| **RAII** | `std::unique_ptr` used throughout data and DB paths; SDL surfaces wrapped in `sdl_surface.hpp`. |
| **Task pipeline** | `TaskSequence` + `Scheduler` separate the game clock from the render clock and allow threading without locks in the hot path. |

---

## Threading model

```
Main thread
  └── SDL event loop  →  UserEventManager  →  handlers

WorkerThread 0  (graphicsSequence)
  └── GraphicsSystem::Task::Process()  →  OpenGL draw calls
      (macOS: all GL calls dispatched here from the main thread)

WorkerThread 1  (gameSequence)
  └── MenuTask::Process()   – UI input / layout
  └── GameTask::Process()   – match simulation tick
      ├── TeamAIController::Update()
      ├── Player::Update()  (×22)
      └── BallPhysics::Update()
```

Shared mutable state (scene graph, resource caches) is protected by the
`Lockable` mixin (`src/types/lockable.hpp`).  Hot-path game objects avoid
locking by owning their data exclusively on the game thread.

---

## Build system

The project uses **CMake 3.14+** with target-based dependency propagation.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Key CMake options:

| Option | Default | Effect |
|---|---|---|
| `GAMEPLAYFOOTBALL_BUILD_GAME` | `ON` | Build the full SDL/OpenGL game binary. |
| `BUILD_TESTING` | `OFF` | Build and register Google Test suites. |
| `GAMEPLAYFOOTBALL_BUILD_DOCS` | `OFF` | Add a `docs` CMake target that runs Doxygen. |

Object libraries (`gf_mathlib`, `gf_ballphysicslib`, `blunted2`, …) allow the
headless test builds to compile and link only the code they need without
pulling in SDL/OpenGL headers.

See `sources.cmake` for the complete source-file list.

---

## Data assets

Runtime assets live in `data/` and must be copied next to the executable:

```
data/
├── databases/default/   # SQLite database, team logos, kit textures
├── locale/              # Localisation strings (en, es, fr, de, pt)
└── media/
    ├── animations/      # Player animation keyframes
    ├── fonts/           # TTF fonts used by SDL_ttf
    ├── objects/         # 3-D stadium and kit .object/.ase models
    ├── shaders/         # GLSL vertex and fragment shaders
    ├── sounds/          # WAV effects and crowd loops
    └── textures/        # Pitch, sky and UI textures
```

After running CMake configure, copy (or symlink) the data directory:

```bash
cp -R data/ build/
# or
ln -s "$(pwd)/data" build/data
```

The helper script `scripts/setup_assets.sh` automates this step.

---

## Testing strategy

Tests live in `tests/` and use **Google Test** (fetched via CMake
`FetchContent`).

| Suite | Target | What it validates |
|---|---|---|
| `base_math_tests` | `tests/base/` | Vector3, Matrix3, Quaternion, bluntmath |
| `ball_physics_tests` | `tests/onthepitch/` | BallPhysics trajectories and bounce |
| `velocity_state_tests` | `tests/onthepitch/` | Player sprint/fatigue state machine |
| `integration_match_test` | `tests/integration/` | Full 90-second headless match |

Headless mode (`-DGAMEPLAYFOOTBALL_BUILD_GAME=OFF`) compiles only
`gf_mathlib` and `gf_ballphysicslib`, so no SDL/OpenGL/OpenAL headers or
display server are required. CI runs these tests on every push.

```bash
cmake -S . -B build-test \
  -DGAMEPLAYFOOTBALL_BUILD_GAME=OFF \
  -DBUILD_TESTING=ON
cmake --build build-test --parallel
ctest --test-dir build-test --output-on-failure
```
