# CS202 — Game Project Report

> **Course:** CS202 — Object-Oriented Design | **Group:** 02 | **Date:** 2026-08-28
> **Members:** [Student 1 — ID], [Student 2 — ID], [Student 3 — ID] <!-- TODO: fill -->
> **Instructor:** [Name] <!-- TODO: fill -->
> **Repository:** `CS202_GameProject` (SFML 3 + Box2D 3.1)

---

## 1. Introduction

This project is a 2D side-scrolling platform game inspired by classic Mario gameplay, implemented in C++20 with SFML 3 for graphics, audio, windowing, and input, and Box2D 3.1.1 for physics and collision handling. Players navigate themed levels, defeat enemies, collect coins and power-ups, activate checkpoints, and reach the flagpole while managing score, lives, and a time limit. The game supports solo play, local cooperative play, and a battle-style minigame for two local players or one player against a heuristic AI opponent. An in-game map editor and JSON-based level and prefab data make it possible to create and test custom layouts without changing the core gameplay code. The project is intended for players who enjoy short, replayable platforming challenges and for demonstrating object-oriented game architecture in an educational desktop application.

### 1.1 Project Overview

The application begins at a scene-based main menu where players can start or continue a default campaign, select a play mode and character, open settings and leaderboards, or enter the map editor. Default play contains three campaign levels with grassland, underground, and castle-themed content; levels include terrain, slopes, pipes and warps, Goombas, Koopas, Piranha Plants, blocks, coins, power-ups, checkpoints, and flagpole goals. Players may play alone as Mario or Luigi, or play locally with Mario and Luigi together. Minigame mode loads a dedicated arena for two-player competition or a Mario-versus-heuristic-AI match. Campaign sessions persist score, coins, lives, time, checkpoint progress, destroyed tiles, and relevant object state in JSON save files, while completed runs contribute to a persistent leaderboard. The map editor supports placing supported terrain and gameplay objects, validates player spawns, saves a custom JSON map, and allows the result to be playtested in the game.

---

## 2. System Overview

The system is organized into the following cooperating subsystems:

- **Application and timing:** `App` owns the SFML window and main loop, polls input events, separates simulation from visual updates, and caps rendering at 60 FPS.
- **Scene management:** `SceneManager` maintains a stack of screens, while `SceneFactory` creates menus, settings, level and character selection, gameplay, save/load, score, leaderboard, and editor scenes.
- **Gameplay world:** `InGameScene` coordinates the active `GameWorld`, camera, HUD, score, pause flow, win conditions, and game-over handling.
- **Map and content loading:** `LevelDataLoader`, `PrefabRegistry`, and `PrefabSpawner` read JSON level data, resolve reusable prefab definitions, build tile terrain, and create gameplay objects from map symbols and placements.
- **Entities and behaviours:** `GameObject` provides the common entity interface for players, enemies, blocks, items, pipes, and projectiles; reusable behaviours, controllers, and player state objects add movement, animation, attacks, damage, power-ups, and AI.
- **Physics and interaction:** `PhysicsWorld` advances Box2D bodies, collision filters, contacts, and sensors; `WorldInteractionSystem` translates those events into gameplay effects such as damage, item collection, enemy defeat, warps, and block destruction.
- **Presentation and persistence:** SFML renders the world, camera view, animations, HUD, effects, and UI, while `ResourceManager` and audio managers provide shared assets; `SaveLoadGame`, `GameSettings`, `ScoreManager`, and `LeaderboardManager` provide JSON-backed session, configuration, scoring, and leaderboard data.

```text
+------------------------------+
| App                          |
| SFML window, events, timing  |
+---------------+--------------+
                |
                v
+------------------------------+
| SceneManager                 |
| scene stack + SceneFactory   |
| + deferred transitions       |
+---------------+--------------+
                |
                v
+------------------------------+
| Current Scene                |
| menus, editor, InGameScene  |
+---------------+--------------+
                |
                v
+------------------------------+
| GameWorld                    |
| map + object store           |
+---------------+--------------+
                |
        +-------+-------+
        |               |
        v               v
+---------------+ +---------------+
| WorldMap      | | GameObject    |
| tiles         | | entities      |
+-------+-------+ +-------+-------+
        |                 |
        +--------+--------+
                 |
                 v
+------------------------------+
| Box2D physics, behaviours,   |
| animation, interaction,      |
| and rendering                |
+------------------------------+

JSON maps/prefabs ──▶ LevelDataLoader / PrefabRegistry ──▶ GameWorld
```

### 2.1 Architecture Layers

The top-level control flow is a two-rate loop. `App::run()` polls SFML events, adds the measured frame time to a `FixedStepAccumulator`, consumes zero or more fixed simulation steps, updates visual-only systems using the current frame delta, and renders the frame. During gameplay, the fixed-step path is `App → SceneManager → InGameScene → GameWorld`; the accumulator supplies a simulation delta of 1/60 second, so movement, game rules, and physics are not tied directly to variable render-frame duration. `PhysicsWorld` further advances Box2D using its configured internal substeps, then buffered contact and sensor events are processed by the world interaction system.

The architecture follows a layered ownership model. The application layer owns the window and timing; the scene layer owns screen-specific input, UI, lifecycle, and transitions; the world layer owns the loaded map, entities, object creation, interactions, camera-facing gameplay state, and rendering coordination; and the entity layer models individual actors through the polymorphic `GameObject` base class. Physics bodies and collision data are attached to entities through the physics layer, while animation, audio, and SFML drawing provide the presentation layer. This separation allows menu and editor screens to reuse the same application and scene infrastructure while gameplay scenes reuse world services for campaign levels, custom maps, and minigames.

---

## 3. Class Diagram

### 3.1 Main Architecture

```text
App
 └── SceneManager
      └── Scene
           ├── Menu / Settings / Editor scenes
           └── InGameScene
                └── GameWorld
                     ├── WorldMap
                     ├── WorldObjectStore
                     │    └── GameObject
                     │         ├── Player
                     │         ├── Enemy
                     │         ├── Block
                     │         ├── Item
                     │         ├── Fireball
                     │         └── KoopaShell
                     ├── PhysicsWorld
                     └── Rendering / Interaction
```

### 3.2 Class Diagram

![Class Diagram](class_diagram.png)

---

## 4. Applied Design Patterns

<!-- TODO: Fill the table with 4-6 patterns you actually applied. Keep reasoning short here; expand one pattern in 4.1. -->

| # | Pattern | Where Applied | Problem Solved |
|---|---------|---------------|----------------|
| 1 | Decorator | `MegaStateDecorator`, `StarManStateDecorator` | Temporary power-ups via runtime wrapping |

### 4.1 Pattern in Detail — [State]

<!-- TODO: Pick ONE pattern to elaborate. Duplicate this subsection for each pattern if the teacher expects detail. -->
<!-- - Context / Problem: -->
<!-- - Solution in our code (file:line): -->
<!-- - Consequences / Alternatives rejected and why: -->

**Context:**

<!-- TODO -->

**Solution:**

<!-- TODO: e.g., Scene defines init()/onEnter()/onExit()/update()/render(); SceneManager holds stack<unique_ptr<Scene>> ... -->

**Why not alternatives:**

<!-- TODO: e.g., enum + switch would violate OCP; adding a scene would require modifying the central switch. -->

---

## 5. Design Reasoning

<!-- TODO: Explain WHY you designed it this way (§4 says WHAT). 6-10 bullets. -->

### 5.1 Principles

<!-- TODO: -->
<!-- - SRP: Each package owns one change axis (Scene vs World vs Entity). -->
<!-- - OCP: New scene/item via factory registration, no core edits. -->
<!-- - DIP: SceneManager depends on Scene abstraction, not concretes. -->
<!-- - Composition over Inheritance: GameObject + Behaviour avoids deep hierarchy. -->

### 5.2 Key Decisions & Trade-offs

<!-- TODO: 3-5 decisions -->
<!-- - Fixed vs variable timestep (deterministic physics, subSteps=12). -->
<!-- - Singleton for ResourceManager/GameSettings (global cache vs testability). -->
<!-- - Data-driven prefabs (SpawnSpec + PrefabRegistry) for level extensibility. -->
<!-- - Object Pool for Fireball (allocation vs bound). -->

### 5.3 Limitations & Future Work

<!-- TODO: What would you improve next? -->

---

## 6. Conclusion

<!-- TODO: 1 paragraph — what was achieved, what was learned, result. -->

---

## Appendix

---

## References

<!-- TODO: Add references if needed. -->
<!-- - SFML 3 Documentation: https://www.sfml-dev.org/ -->
<!-- - Box2D 3.1 Documentation: https://box2d.org/ -->
<!-- - Gamma et al., Design Patterns: Elements of Reusable Object-Oriented Software -->
