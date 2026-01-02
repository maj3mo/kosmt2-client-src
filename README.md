# Client Source Repository

This repository contains the source code necessary to compile the game client executable.

## How to build

> cmake -S . -B build
>
> cmake --build build

---

## 📋 Changelog

### 🐛 Bug Fixes
* **Invisibility:** Resolved an issue where effects were not visible after becoming visible again, if they were added in AFFECT_INVISIBILITY state.
* **Invisibility:** Resolved an issue where projectile fly effects were visible on targets within AFFECT_INVISIBILITY state.
* **Effects on low opacity meshes**: Resolved a conflict between effects on meshes with opacity < 1 and invisibility fixes.
