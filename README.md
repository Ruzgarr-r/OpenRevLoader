# OpenRevLoader

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%20(x86%2Fx64)-lightgrey.svg)](https://microsoft.com)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C.svg)](https://isocpp.org)

**OpenRevLoader** is a opensource lightweight Steam emulator that runs games require Steam, without Steam! btw its just a fork of revLoader's revEmu but still, its pretty lightweight and useful.

<sub>a Garry's Mod instance thats being runned by OpenRevLoader</sub>
<img width="1920" height="1073" alt="image" src="https://github.com/user-attachments/assets/166d41ee-2d98-428f-acb3-b84f6fa75ddb" />

Things that i added to this fork:
- Account selector when launching the game
- Overlay DLL option (removed)


<sub>Half Life 2 being runned by OpenRevLoader</sub>
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/0d135495-bab7-42ac-90ca-e4f8a3edf582" />

## Building OpenRevLoader:

### MinGW / GCC
```bash
# 32-bit (x86):
g++ -O2 -s revLoader.cpp -o OpenRevLoader.exe -luser32 -lshell32 -lgdiplus -lgdi32 -lole32 -m32

# 64-bit (x64):
g++ -O2 -s revLoader.cpp -o OpenRevLoader.exe -luser32 -lshell32 -lgdiplus -lgdi32 -lole32 -m64
