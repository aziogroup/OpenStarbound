# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenStarbound is an open-source fork of Starbound 1.4.4 (2D sandbox game). It fixes bugs, adds features, and improves performance. The game requires a copy of vanilla Starbound's `packed.pak` in the `assets/` directory to run. This is a personal extended fork.

## Build Commands

The CMake project root is `source/` (not the repo root). All commands run from `source/`.

### Windows (primary development platform)

#### Environment Setup

- **VCPKG_ROOT**: `G:/Development/vcpkg` — must be set before `cmake --preset`
- **Generator**: VS2022 multi-config (`windows-release-VS2022` preset). The Ninja preset (`windows-release`) requires Developer Command Prompt with Ninja/compiler in PATH.
- **Build directory**: `build/windows-release/` — shared by both presets. **Never mix generators** in the same build dir. If switching generators, delete `CMakeCache.txt` and `CMakeFiles/` first.

**CRITICAL**: You **MUST** always specify `--config RelWithDebInfo` on build commands. Without it, VS2022 defaults to Debug, which causes: abort() crashes from debug assertions, 20x slower startup, and other runtime failures. **NEVER** omit `--config`.

```bash
# Set VCPKG_ROOT if not already set
export VCPKG_ROOT="G:/Development/vcpkg"

# Configure (VS2022 — works from any terminal)
cmake --preset windows-release-VS2022

# Build (ALWAYS specify --config RelWithDebInfo)
cmake --build --preset windows-release-VS2022 --config RelWithDebInfo

# Build a specific target
cmake --build --preset windows-release-VS2022 --config RelWithDebInfo --target starbound

# Alternative: Ninja preset (requires Developer Command Prompt)
cmake --preset windows-release
cmake --build --preset windows-release
```

### Linux

```bash
cmake --preset linux-release        # GCC
cmake --preset linux-release-clang  # Clang
cmake --build --preset linux-release
```

### macOS

```bash
cmake --preset macos-release       # Intel
cmake --preset macos-arm-release   # Apple Silicon
cmake --build --preset macos-release
```

### Build output

Binaries go to `dist/` at the repo root. Executables: `starbound` (client), `starbound_server`, `asset_packer`, `asset_unpacker`.

## Testing

```bash
# Run tests via preset (only runs core_tests, labeled NoAssets)
ctest --preset windows-release

# Run core_tests directly (no game assets needed)
../dist/core_tests

# Run game_tests directly (requires packed.pak in assets/)
../dist/game_tests

# Run a single test
../dist/core_tests --gtest_filter=JsonTest.*
```

- Framework: Google Test (vendored in `source/test/gtest/`)
- `core_tests`: ~40 tests for core library, labeled `NoAssets` - safe to run without game assets
- `game_tests`: ~11 tests for game logic, requires game assets to run
- CI only runs `core_tests` (filtered by `NoAssets` label)

## Architecture

### Module Dependency Hierarchy

All modules are OBJECT libraries linked statically into final executables. Strict layered dependency - each layer only depends on layers below it.

```
extern     vendored C/C++ (Lua 5.2, fmt, xxhash, curve25519, imgui bindings, rpmalloc)
  |
core       general-purpose foundation (~212 files) - data structures, I/O, JSON, threading,
  |        networking (NetElement), Lua engine wrapper, image processing, crypto
  |
base       game support (~26 files) - asset system, cellular lighting/liquid,
  |        mixer, world geometry, configuration
  |
platform   headers-only interfaces - P2P networking, statistics, UGC services
  |
game       core game logic (~499 files) - THE LARGEST MODULE
  |        entities, items, world (client/server), universe (client/server),
  |        terrain generation, networking packets, ~30 databases, Lua bindings
  |
application  SDL3 main loop, OpenGL renderer, platform service implementations
  |
rendering    2D rendering - tiles, environment, text, drawables
  |
windowing    UI widget system - panes, buttons, labels, scroll areas, layouts
  |
frontend     client interface (~102 files) - HUD, menus, chat, voice, cinematics
```

### Executable Composition

| Executable | Modules |
|---|---|
| `starbound` (client) | all modules |
| `starbound_server` | extern + core + base + game (no GUI) |
| `asset_packer/unpacker` | extern + core + base |
| `core_tests` | extern + core |
| `game_tests` | extern + core + base + game |

### Key Singletons and Systems

- **`Root`** (`game/StarRoot`): Central game singleton providing lazy-loaded, mutex-protected access to ~30 databases. `RootLoader` handles CLI args and boot config (`sbinit.config`).
- **`Assets`** (`base/StarAssets`): Thread-safe asset manager with TTL caching and background workers. Supports JSON/image patching and Lua asset scripts. Sources layered in order (later overrides earlier).
- **`LuaRoot`** (`game/scripting/StarLuaRoot`): Manages Lua engine, script caching from assets, and context creation. Both `UniverseClient` and `UniverseServer` host separate instances.

### Networking

- **Transport**: `LocalPacketSocket` (single-player, in-process queues), `TcpPacketSocket` (TCP + ZSTD), `P2PPacketSocket` (Steam/Discord + ZSTD)
- **State sync**: `NetElement` system in core - composable delta-based sync with interpolation support, monotonic version tracking
- **Protocol**: ~40+ custom packet types in `StarNetPackets.hpp`

### Lua Scripting

Lua is deeply integrated with bindings at every layer:
- `core/scripting/`: utility bindings
- `base/scripting/`: image manipulation
- `game/scripting/` (~24 files): world, entity, player, item, status, movement, celestial, camera, input, config, behavior, HTTP, etc.
- `frontend/`+`client/`: widget, interface, clipboard, voice, rendering bindings

Script components hierarchy: `LuaBaseComponent` -> `LuaUpdatableComponent` -> `LuaWorldComponent` -> `LuaMessageHandlingComponent`

### Asset/Mod System

Asset sources (in override order): `DirectoryAssetSource` (filesystem) -> `PackedAssetSource` (.pak) -> `MemoryAssetSource` (runtime). Later sources override earlier ones. JSON patching via JSON Patch operations or `.patch.lua` scripts.

## Code Conventions

- **C++17**, namespace `Star`
- All source files prefixed with `Star` (e.g., `StarPlayer.cpp`, `StarWorldClient.hpp`)
- `STAR_CLASS(Foo)` macro for forward declarations with shared/unique pointer typedefs
- `STAR_EXCEPTION(FooException, StarException)` for exception types
- Formatting: `.clang-format` - LLVM base, 2-space indent, no tabs, no column limit, K&R braces, pointer left-aligned (`int* p`)
- Custom data structures preferred over STL (`StarList`, `StarMap`, `StarSet`, `StarString`, `StarMaybe` instead of `std::optional`)
- Platform-specific code in `_unix.cpp` / `_windows.cpp` suffixed files

## Key CMake Options

| Option | Default | Notes |
|---|---|---|
| `STAR_BUILD_GUI` | ON | Gates client/GUI modules |
| `STAR_ENABLE_STEAM_INTEGRATION` | OFF (ON in presets) | Steam platform services |
| `STAR_ENABLE_DISCORD_INTEGRATION` | OFF (ON in presets) | Discord platform services |
| `STAR_USE_RPMALLOC` | OFF | Used on Windows (in presets) |
| `STAR_USE_JEMALLOC` | OFF | Used on Linux (in presets) |
| `BUILD_TESTING` | OFF (ON in presets) | Build test executables |
| `STAR_LUA_APICHECK` | OFF | Lua debug checks |

## ImGui Integration

ImGui (Dear ImGui) is fully integrated with SDL3 + OpenGL3 backends. Use it for debug UI, development tools, and in-game parameter tuning.

### Architecture

- **Setup**: `StarMainApplication_sdl.cpp` — `ImGui::CreateContext()`, `ImGui_ImplSDL3_InitForOpenGL()`, `ImGui_ImplOpenGL3_Init()`
- **Frame cycle**: `NewFrame()` is called per update tick, `ImGui::Render()` + `ImGui_ImplOpenGL3_RenderDrawData()` after `finishFrame()`
- **Input**: `ImGui_ImplSDL3_ProcessEvent()` handles all SDL events automatically
- **Font**: Custom font loaded from `/hobo.ttf` in `ClientApplication::renderInit()`

### How to Add Debug Windows

Add ImGui code in `ClientApplication::render()` (after game rendering, before error screen). Example pattern:

```cpp
#include "imgui.h"

// In ClientApplication::render():
{
  static bool showWindow = false;
  if (ImGui::IsKeyPressed(ImGuiKey_F5, false))  // Toggle with F5
    showWindow = !showWindow;

  if (showWindow) {
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug Window", &showWindow)) {
      // Checkboxes, sliders, text, etc.
      static float myParam = 1.0f;
      ImGui::SliderFloat("Parameter", &myParam, 0.0f, 2.0f);

      // Set shader parameters via renderer
      auto& renderer = Application::renderer();
      renderer->setEffectScriptableParameter("effectName", "paramName", myParam);

      // Read/write game configuration
      auto config = m_root->configuration();
      bool flag = config->get("myFlag").optBool().value(false);
      if (ImGui::Checkbox("My Flag", &flag))
        config->set("myFlag", flag);
    }
    ImGui::End();
  }
}
```

### Used Keys

| Key | Purpose |
|---|---|
| F3 | Graphics Enhancement debug window (bloom, AO, color bleed, GPU shadows) |

### Guidelines

- Use F-keys (F3-F12) for toggling debug windows to avoid gameplay key conflicts
- Use `static` variables for window state and parameter values
- Use `renderer->setEffectScriptableParameter()` to control shader params from ImGui (works across effect switches)
- Use `m_root->configuration()->get()/set()` for persistent settings (saved to `starbound.config`)
- Always gate debug windows behind a key toggle — don't show by default
- Prefer ImGui for all new debug/development UI over the game's widget system

## Session Management

- Context is 1M tokens - do NOT suggest session breaks or compaction preemptively
- The user will stop when needed - continue working until told otherwise
- Do not ask "should we stop?" or "should we save progress?" - just keep going

## Documentation

- `doc/lua/`: Vanilla Starbound Lua API reference (~30 files)
- `doc/lua/openstarbound/`: OpenStarbound-specific Lua API extensions (~24 files)
- `doc/json/`: JSON configuration format docs
