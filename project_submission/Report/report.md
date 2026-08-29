# CS202 — Game Project Report

> **Course:** CS202 — Object-Oriented Design | **Group:** 02 | **Date:** 2026-08-28
> **Members:** [Student 1 — ID], [Student 2 — ID], [Student 3 — ID] <!-- TODO: fill -->
> **Instructor:** [Name] <!-- TODO: fill -->
> **Repository:** `CS202_GameProject` (SFML 3 + Box2D 3.1)

---

## 1. Introduction

<!-- TODO: Write 4-6 sentences. What is the game? Goals? Scope? Target users? -->
<!-- Example: This project is a 2D Mario-like platformer that supports ... -->

### 1.1 Project Overview

<!-- TODO: 1 paragraph — game genre, main features (single/multiplayer, level editor, save/load, minigame). -->

---

## 2. System Overview

<!-- TODO: 5-8 bullets + 1 high-level architecture figure. Keep it simple. -->

### 2.1 Architecture Layers

<!-- TODO: Describe the layering: App (loop) → Scene (screens) → World (map + entities) → Entity (GameObject) → Physics/Animation. -->
<!-- Mention fixed-timestep loop: App → SceneManager → GameWorld (1/60 s). -->

---

## 3. Class Diagram(s)

<!-- INSTRUCTION: Paste 1-2 SIMPLIFIED mermaid diagrams here (10-15 classes max). -->
<!-- GitHub renders ```mermaid natively; no extension needed on github.com. -->

### 3.1 Overview Diagram

<!-- TODO: Replace the example below with your chosen overview (e.g., App → Scene → GameWorld → GameObject). -->

```mermaid
classDiagram
    class App {
        -SceneManager manager
        -SceneFactory factory
        +run()
    }
    class Scene {
        <<abstract>>
        +update()
        +render()
    }
    class SceneManager {
        -stack~Scene~ sceneStack
        +pushScene()
        +popScene()
    }
    class GameWorld {
        -WorldMap worldMap
        -WorldObjectStore store
        +loadLevel()
        +update()
    }
    class GameObject {
        <<abstract>>
        -PhysicsBody body
        +spawn()
        +onContact()
    }

    App *-- SceneManager
    SceneManager "1" *-- "many" Scene
    Scene <|-- InGameScene
    InGameScene *-- GameWorld
    GameWorld *-- GameObject
```

*Fig. 1 — High-level overview (simplified). See Appendix for detailed subsystem diagrams.*

### 3.2 Detailed Diagram (Optional)

<!-- TODO: Add ONE detailed diagram if required (e.g., Player State or World pipeline). Otherwise delete this subsection. -->
<!-- ```mermaid -->
<!-- classDiagram -->
<!--     class Player { ... } -->
<!-- ``` -->

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
