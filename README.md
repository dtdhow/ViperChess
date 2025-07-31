# ViperChess-Mega ♟️⚡

## Please don't mind the current spaghetti code, so just pull request if you can fix that.

An UCI-compatible chess engine written in C++.

## Features
- **UCI protocol** support (works with Arena, CuteChess, etc.)  
- **Custom eval** (material, pawn structure, king safety, etc.)  
- **Opening book** support (Polyglot .bin)  

## Build
```sh
git clone https://github.com/yourname/ViperChess-Mega
cd ViperChess-Mega
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
## Usage
```sh
./viperchess-mega # UCI mode
```
## Or in GUIs: add engine -> select executable
