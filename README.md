# OpenRevLoader

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%20(x86%2Fx64)-lightgrey.svg)](https://microsoft.com)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C.svg)](https://isocpp.org)

**OpenRevLoader** (formerly *revLoader*) is an open-source, modernized C++ loader and Steam emulation bridge designed for Valve's Source Engine games (such as *Team Fortress 2*, *Garry's Mod*, *Counter-Strike: Source*, and standalone mods).

It handles process spawning, shared memory initialization, Steam registry keys, AppID resolution, and dynamic DLL injection for Steam Overlay (`GameOverlayRenderer.dll`) and `revEmu`.

---

##  Key Features:

- **Dynamic DLL Injection Engine**: Automatically injects `revEmu.dll`, `GameOverlayRenderer.dll`, or custom libraries into the suspended target process before thread execution begins.
- **Robust AppID Resolution**: Automatically resolves Steam AppIDs using a multi-tiered fallback hierarchy (`steam_appid.txt` -> `rev.ini` `[Steam]` -> `rev.ini` `[Loader]`).
- **Flexible Command Line Parsing**: Pass custom flags (`-launch`, `-appid`, etc.) directly via CLI or configure them inside `rev.ini`.
- **Registry & Environment Injection**: Registers active Steam process details (`ActiveProcess` registry keys) and populates `SteamAppId`, `SteamGameId`, `SteamOverlayGameId`, and `SteamPath` environment variables.
- **Improved Working Directory Pathing**: Ensures relative file lookups resolve correctly regardless of invocation.
- **Non-blocking Dependencies**: Optional handling of patched DLL files.

---

##  Building OpenRevLoader:

### MinGW / GCC
```bash
# 32-bit (x86):
g++ -O2 -s revLoader.cpp -o OpenRevLoader.exe -luser32 -lshell32 -m32

# 64-bit (x64):
g++ -O2 -s revLoader.cpp -o OpenRevLoader.exe -luser32 -lshell32 -m64
