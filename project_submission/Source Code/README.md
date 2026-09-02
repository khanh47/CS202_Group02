# CS202 Game Project

## Overview

This project features a Mario 2D platformer written in C++20 (educational purpose). It uses
[SFML 3](https://www.sfml-dev.org/) for graphics, audio, windowing, and input,
and [Box2D 3.1.1](https://box2d.org/) for physics and collision handling.

The project includes campaign and local multiplayer play, a heuristic-AI
opponent, an in-game map editor, and JSON-driven levels and prefabs. It is
released under the [MIT License](LICENSE).

## Installation

### Prerequisites

- Git
- CMake 3.22 or newer
- Ninja
- A compiler with C++20 support

Windows development is verified with the MSYS2 UCRT64 MinGW toolchain. From an
MSYS2 UCRT64 shell, the required development tools can be installed with:

```sh
pacman -S --needed git mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

The repository vendors SFML 3.0.2, Box2D 3.1.1, and nlohmann JSON for the
Windows build. On Unix, Box2D and nlohmann JSON remain vendored, while the
current CMake configuration expects SFML 3 under the Linux Homebrew prefix
`/home/linuxbrew/.linuxbrew`.

Clone the repository and enter the project directory:

```sh
git clone https://github.com/khanh47/CS202_GameProject.git
cd CS202_GameProject
```

## Building

Configure and build the `Mario` target from the repository root:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target Mario -j 8
```

CMake automatically copies `assets/` into the build directory. The resulting
executable is `build/Mario.exe` on Windows and `build/Mario` on Unix.

## Running

Run the game from the repository root so its asset and save paths resolve
consistently.

Windows PowerShell:

```powershell
.\build\Mario.exe
```

Unix shell:

```sh
./build/Mario
```

The game opens at the main menu. Controls and key bindings can be viewed and
changed from **Settings**. Save files are stored in `assets/SaveGameFiles/`,
and the map editor writes its custom level to
`assets/datas/levels/custom-map.json`.

## Project Structure

```text
CS202_GameProject/
|-- include/       Public and internal C++ headers
|-- src/           C++ implementations and the application entry point
|-- assets/        Levels, prefabs, sprites, audio, fonts, shaders, and saves
|-- external/      Vendored SFML, Box2D, and nlohmann JSON dependencies
|-- tests/         Regression and subsystem test sources
|-- docs/          Supporting project documentation
|-- CMakeLists.txt CMake configuration for the Mario executable
`-- README.md       Project setup and feature overview
```

Both `include/` and `src/` are organized into the main `Animation`, `Audio`,
`Button`, `Commands`, `Game`, `Physics`, and `Scene` subsystems. Gameplay code
under `Game` is further separated into AI, behaviours, objects, snapshots,
input handling, and world management.

## Features

- Campaign levels with solo and local co-op character selection
- Two-player battle minigames and Mario-versus-heuristic-AI mode
- In-game map editor with JSON level and prefab data
- Save, load, continue, and checkpoint support
- Box2D-based movement, contacts, sensors, projectiles, and destructible terrain
- Sprite-sheet animation, reusable behaviours, and player state transformations
- Mario and Luigi movement characteristics and configurable controls
- Enemies, shells, coins, blocks, power-ups, and temporary invincibility states
- Pipes, directional pipe segments, warps, checkpoints, and flagpole victories
- Score, lives, floating score text, win sequences, and game-over flow
- Music, sound effects, shaders, particles, and parallax backgrounds
- Configurable audio, key bindings, camera options, and debug visualization
