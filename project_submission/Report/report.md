# CS202 — Game Project Report

> **Course:** CS202 — Programming Systems | **Group:** 02 | **Date:** 2026-08-28
> **Members:** [Võ Gia Minh — 25125030], [Phạm Huy Khánh — 25125055], [Nguyễn Quốc Thịnh — 25125066], [Chu Nguyễn Gia Khánh - 25125085]
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

- The main goal of our architecture was to make the gameplay systems easier to extend while keeping the physics stable and object ownership clear. Instead of putting all the logic into one large class, we separated the program into different systems for scenes, world simulation, physics, rendering, input, animation, and object behaviours. This makes the code easier to follow and lets us modify one system without affecting too many unrelated parts.

- During development, our group tried to follow the SOLID principles when making design decisions. For example, we used abstract interfaces for scenes, separated physics from rendering, and used behaviours for abilities that are not needed by every object. Despite this phylosophy, the design changed many times as we added more features, so not every part of the project follows these principles perfectly.

### 5.1 Principles

- Single Responsibility Principle (SRP): We tried to give each major class one main responsibility. `SceneManager` manages scene transitions, `GameWorld` coordinates the gameplay systems, `PhysicsWorld` handles the Box2D simulation, `WorldRenderer` renders the world, and `WorldInteractionSystem` processes contact and sensor events. This prevents all of these responsibilities from being placed inside one large class.

- Open/Closed Principle (OCP): The project can be extended with new scenes, game objects, animations, and level prefabs without rewriting most of the existing code. We use inheritance, factories, registries, and JSON definitions to support this. For example, a level object can be added as a prefab and spawned through the existing map-loading system.

- Liskov Substitution Principle (LSP): Concrete classes can generally be used through their base interfaces. `SceneManager` can manage different classes derived from `Scene`, while the player can use different implementations of `PlayerState`. This means the main program flow does not need separate logic for every concrete implementation.

- Interface Segregation Principle (ISP): We use focused abstractions such as `Behaviour` and `IAttackStrategy` instead of putting every possible operation into one large interface. This means an object only needs to use the functionality that is relevant to it. For example, attack logic is kept separate from movement and animation.

- Dependency Inversion Principle (DIP): High-level systems use abstractions when possible. `SceneManager` works with the abstract `Scene` interface instead of directly handling every concrete scene type. Player attack logic also uses `IAttackStrategy`, which allows the attack implementation to change without rewriting the main player logic.

- Composition over inheritance: Optional abilities such as animation, movement, damage handling, invincibility, visual effects, and shell holding are attached to a `GameObject` as behaviours. This lets each object have only the abilities it needs. It also allows temporary behaviours to be added or removed during gameplay without creating many subclasses for every possible combination.

### 5.2 Key Decisions & Trade-offs
#### Behaviour Composition

- Optional abilities are stored in `GameObject::_behaviours` as a `vector<unique_ptr<Behaviour>>`. Objects can use `addBehaviour<T>()`, `getBehaviour<T>()`, and `removeBehaviour<T>()` to change their abilities at runtime. For example, temporary invincibility can be attached to a player and removed later without adding another level to the `Player` inheritance hierarchy.

- The trade-off is that behaviours are found using a linear search and `dynamic_cast`. This is simple for the small number of behaviours owned by each object, but it would become less efficient if objects had many behaviours. Removing a behaviour also requires care because other systems may still depend on it.

#### Data-Driven Levels and Prefabs

- Levels and reusable objects are defined through JSON. Map symbols refer to entries in `PrefabRegistry`, which resolves them into `SpawnSpec` values. These values can configure the object type, texture, animation, size, collision properties, pipe warps, and contained items. This allows us to create and modify levels without hard-coding every object placement in C++.

- The trade-off is that JSON errors are only discovered when the data is loaded. Invalid prefab names, missing properties, or incorrect values cannot be detected by the C++ compiler. Therefore, the project needs careful data validation and clear loading errors.

#### Scene Stack

- `SceneManager` owns a `stack<unique_ptr<Scene>>` and changes screens using push, pop, and replace operations. This gives menus, settings, save screens, level selection, and gameplay a common navigation system. Scene changes are deferred until the current operation finishes, preventing a scene from being destroyed while it is still processing an event.

- The trade-off is that only the top scene is treated as the active screen. Transparent overlays are not fully supported as separate scenes, so the pause menu, victory screen, and game-over screen are managed inside `InGameScene`.

#### Player States and Decorators

- Normal, Super, and Fire forms are represented through the `PlayerState` interface. Temporary effects such as Mega and Star Man use `PlayerStateDecorator` to wrap the current state. This preserves the underlying form while adding temporary properties such as a larger scale, invincibility, or modified movement.

- The trade-off is that wrapped states are more difficult to inspect and update than a single state value. Some parts of the program use `dynamic_cast` to check which decorator is active, and care must be taken when adding, removing, or restoring nested states.

### 5.3 Limitations & Future Work
- Although the project applies object-oriented principles, the architecture does not follow them perfectly in every part of the program. As more gameplay features were added, some classes gradually gained more responsibilities than originally intended. This makes these classes more difficult to understand, modify, and test. In future development, these responsibilities could be divided into smaller and more focused classes to improve maintainability and follow the Single Responsibility Principle more closely.

- Some complex systems (Player, GameWorld) also depend directly on concrete classes or globally accessible managers. This creates stronger coupling because a change to one class may require changes in other parts of the program. A better approach would be to introduce more focused abstractions and provide dependencies through constructors. This would make individual systems easier to replace, extend, and test independently.

- The behaviour and player-state systems successfully reduce the need for deep inheritance hierarchies, but they also introduce additional runtime type checking and coordination between objects. Future improvements could provide clearer interfaces for communication between behaviours and reduce the need to identify concrete types. This would help the project follow the Open/Closed and Dependency Inversion Principles more consistently.

- Overall, the current architecture is suitable for the scope of the project and supports its main features. However, further refactoring could improve the consistency of the object-oriented design, reduce coupling between systems, and make the project easier to extend with new gameplay mechanics.

---

## 6. Conclusion

- In conclusion, our group developed a complete 2D Mario-style platformer using C++20, SFML 3, and Box2D 3.1. During development, we learned how object-oriented principles and design patterns can be applied to organize a large program with many connected gameplay systems. We also learned that design decisions involve trade-offs, and that an architecture often needs to be refactored as the project grows. Although some parts can still be improved, the final architecture supports the current features and provides a reasonable foundation for adding new content in the future.
---

## Appendix

---

## References

<!-- TODO: Add references if needed. -->
<!-- - SFML 3 Documentation: https://www.sfml-dev.org/ -->
<!-- - Box2D 3.1 Documentation: https://box2d.org/ -->
<!-- - Gamma et al., Design Patterns: Elements of Reusable Object-Oriented Software -->
