# OpenRevLoader

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%20(x86%2Fx64)-lightgrey.svg)](https://microsoft.com)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C.svg)](https://isocpp.org)

**OpenRevLoader** (formerly *revLoader*) is an open-source, lightweight C++ loader and Steam emulation bridge designed for Valve's Source Engine games (such as *Team Fortress 2*, *Garry's Mod*, *Counter-Strike: Source*, and standalone mods).

It handles process spawning, shared memory initialization, Steam registry key updates, AppID resolution, and essential Steam DLL loading for `revEmu`.

---

## Features:

- **Native Process Execution**: Spawns the target process directly via `CreateProcessA` without thread suspension or remote memory injection.
- **Flexible Command Line Parsing**: Pass custom flags (`-launch`, `-appid`, etc.) directly via CLI or configure the target binary inside `rev.ini`.
- **Direct AppID Resolution**: Automatically resolves the Steam AppID from `steam_appid.txt`.
- **Registry & Environment Sync**: Populates `SteamAppId` and `SteamGameId` environment variables and registers the active process PID under `HKCU\Software\Valve\Steam\ActiveProcess`.
- **Steam Shared Memory Integration**: Sets up `Local\SteamStart_SharedMemFile` and `Local\SteamStart_SharedMemLock` mappings to emulate Steam's internal startup handshake.
- **DLL Initialization**: Loads `SteamClientDll` (if configured in `rev.ini`) and verifies `steam.dll` prior to process execution.

---

##  Building OpenRevLoader:

### MinGW / GCC
```bash
# 32-bit (x86):
g++ -O2 -s revLoader.cpp -o OpenRevLoader.exe -luser32 -lshell32 -m32

# 64-bit (x64):
g++ -O2 -s revLoader.cpp -o OpenRevLoader.exe -luser32 -lshell32 -m64
