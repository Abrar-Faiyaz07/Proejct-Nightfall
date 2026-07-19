# 🌙 The Nightfall

<div align="center">

![Game Title](https://img.shields.io/badge/The%20Nightfall-2D%20Action%20Platformer-blueviolet?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C-blue?style=for-the-badge&logo=c)
![Library](https://img.shields.io/badge/Library-Raylib-orange?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows)
![Status](https://img.shields.io/badge/Status-Complete-brightgreen?style=for-the-badge)

> ✨ **My very first programming project** — a 2D action-platformer and boss shooter built from scratch in C using the Raylib game library!

</div>

---

## 👨‍💻 About the Developer

| | |
|---|---|
| **Name** | Abrar Faiyaz |
| **Institution** | Islamic University of Technology (IUT) |
| **Department** | Computer Science & Engineering (CSE) |
| **Project Type** | First Personal Game Development Project |
| **Technology** | C Language + Raylib Graphics Library |

> 💡 *This project marks the beginning of my journey into game development and systems programming. Built entirely in C with no prior game dev experience — every mechanic, system, and line of code was a learning milestone!*

---

## 🎮 Game Overview

**The Nightfall** is a 2D side-scrolling action platformer where players battle through two dramatically different worlds — a wild jungle and a neon-drenched cyberpunk city. Jump across platforms, climb wooden logs and cyber ladders, dodge deadly hazards, and defeat terrifying bosses to survive the night!

---

## 🌍 Levels

### 🌿 Level 1 — The Jungle
- **Theme**: Dense, mysterious jungle environment with lush parallax backgrounds
- **Platforms**: Stone and mossy platforms scattered across a vast world
- **Hazard**: A giant **Rolling Boulder** that spawns from the right and relentlessly charges forward
- **Boss**: **CORRUPTED JUNGLE BEAST** — a monstrous creature that chases and shoots at the player
- **Climbing**: Wooden log structures provide a tactical climbing mechanic

### 🌆 Level 2 — Cyberpunk City
- **Theme**: Futuristic neon cyberpunk cityscape with glowing backgrounds
- **Platforms**: Cyberpunk-styled platforms and neon glowing ladder structures
- **Hazard**: A blazing **Laser Wall** that sweeps across the screen
- **Boss**: **CYBERNETIC WRAITH** — a high-tech enemy with aggressive shoot patterns
- **Climbing**: Procedurally drawn neon cyan rungs for vertical traversal

---

## ✨ Features

- 🗺️ **Massive Scrolling World** — Each level spans 10x the window width with procedurally placed platforms
- 📷 **2D Camera with Parallax** — Smooth camera tracking with 50% speed background parallax scrolling
- 🧗 **Tactical Climbing System** — Climb wooden logs and neon ladders; hazards **pause** while you climb!
- 💥 **Combat System** — Shoot projectiles at enemies with directional firing
- 🩺 **Health & Invincibility Frames** — Heart-based HP system with flashing damage immunity
- 👾 **Boss Battles** — Each level has a unique boss with a custom health bar HUD
- ⏱️ **Timed Hazard Spawns** — Bosses, stones, and lasers appear on a delay for pacing
- 🎨 **Themed Asset Reloading** — Platform, floor, and background textures swap dynamically between levels
- 🏁 **Full Game Loop** — Menu → Level 1 → Level Transition → Level 2 → Victory / Game Over screens

---

## 🕹️ Controls

| Action | Key(s) |
|:---|:---|
| **Move Left / Right** | `A` / `D` or `←` / `→` Arrow Keys |
| **Sprint (2x Speed)** | `Shift` + Move Left/Right |
| **Jump** | `Space` |
| **Super Jump (1.5x Height)** | `Shift` + `Space` |
| **Climb Up / Down** | `W` / `S` or `↑` / `↓` Arrow Keys (near climbable surface) |
| **Wall Jump** | `Space` while actively climbing |
| **Shoot** | `F` key or `Left Mouse Click` |
| **Start / Next Screen** | `Space` |
| **Return to Menu** | `R` (on game end screen) |
| **Quit** | `ESC` |

---

## 🛠️ Technical Architecture

### Core Systems

| System | Details |
|:---|:---|
| **Language** | C (C99 standard) |
| **Graphics** | Raylib — Hardware-accelerated 2D rendering |
| **Physics** | Custom gravity + vertical velocity integration |
| **Collision** | AABB rectangle collision with foot-probe platform detection |
| **Camera** | Raylib Camera2D with clamped horizontal tracking |
| **State Machine** | MENU → LEVEL1 → LEVEL1_COMPLETE → LEVEL2_INTRO → LEVEL2 → GAME_END |

### Key Structs

```c
Character      // Player & enemies: position, physics, health, bullets, animation state
Platform       // FLOOR, PLATFORM, or CLIMBABLE_WOOD types
RollingStone   // Boulder hazard with spawn/respawn logic
Laser          // Laser wall hazard with spawn/respawn logic
PlayerHealth   // Hearts, invincibility timer & state
Bullets        // Circular bullet pool with ring-buffer indexing
GameState      // Enum-based finite state machine
```

### Climb Detection Algorithm
The climbing system uses **dual side-probe rectangles** extended from the player body to detect nearby climbable wood surfaces. When contact is detected, hazards freeze, gravity is suppressed, and vertical movement is handed to the `W`/`S` keys.

---

## 📁 Project Structure

```
TheNightfall/
│
├── main.c                           # All game logic (~1026 lines)
├── compile.bat                      # Windows GCC build script
├── game.exe                         # Compiled game binary
│
├── raylib.h                         # Raylib main header
├── raymath.h                        # Raylib math utilities
├── rlgl.h                           # Raylib low-level GL abstraction
├── libraylib.a                      # Raylib static library
├── libraylibdll.a                   # Raylib DLL import library
├── raylib.dll                       # Raylib dynamic library
│
└── Resources/                       # All game assets
    ├── background_lvl1_jungle.png   # Level 1 background
    ├── background_lvl2_cyberpunk.png# Level 2 background
    ├── menu_start_jungle.png        # Main menu splash
    ├── level1_complete.png          # Level 1 clear screen
    ├── level2_start_cyberpunk.png   # Level 2 intro screen
    ├── game_end.png                 # Victory/end screen
    ├── standing.png                 # Player idle sprite
    ├── jumping.png                  # Player jump sprite
    ├── walk1.png                    # Player walk animation
    ├── climbing.png                 # Player climb sprite
    ├── monster_right_1.png          # Boss sprite frame 1
    ├── monster_right_2.png          # Boss sprite frame 2
    ├── platform1.png                # Level 1 platform texture
    ├── platform2.png                # Level 1 platform texture (alt)
    ├── floor.png                    # Level 1 floor tile
    ├── wood.png                     # Climbable wood texture
    ├── stone_rolling.png            # Rolling boulder hazard
    └── laser.png                    # Laser wall hazard
```

---

## 🚀 How to Build & Run

### Prerequisites
- **GCC** (MinGW recommended on Windows)
- **Raylib** DLL and libraries (included in repository)

### Build

Double-click `compile.bat`, or run in a terminal:

```bat
compile.bat
```

Or manually compile with GCC:

```bash
gcc -o game.exe main.c -L. -lraylib -lgdi32 -lwinmm
```

### Run

```bash
game.exe
```

---

## 📸 Screenshots

> *Screenshots can be found in the `Screenshot/` folder.*

---

## 🎯 What I Learned

Building **The Nightfall** as my very first project taught me:

- ✅ The fundamentals of **C programming** — structs, pointers, dynamic memory allocation, and function design
- ✅ How to use a **game loop** and delta-time physics for frame-rate independent movement
- ✅ **Collision detection** using axis-aligned bounding boxes (AABB)
- ✅ Managing **game state** with enums and a finite state machine
- ✅ Loading and rendering **textures, animations, and sprite flipping** with Raylib
- ✅ Implementing a **camera system** with world-space to screen-space mapping
- ✅ Basic **game design** — level pacing, hazard timing, boss mechanics

---

## 🙏 Acknowledgements

- **[Raylib](https://www.raylib.com/)** by Ramon Santamaria (raysan5) — for making C game development approachable and fun
- **Islamic University of Technology (IUT)** — for inspiring me to build real things from the ground up

---

<div align="center">

Made with ❤️ by **Abrar Faiyaz**
*CSE Student, Islamic University of Technology (IUT)*

*"Every expert was once a beginner."*

</div>
