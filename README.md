# Typing the Bomb
A high-performace, real-time arcade typing simulation engine executed entirely within a standard terminal environment. Modeled after classic tactical typing games, the system challenges users to neutralize falling ordnance by processing complex string sequences under dynamic clock regulation.

## Key Features
* **Zero-Overhead Session Serialization:** Direct pointer-free memory persistence to disk.
* **Double-Buffered Rendering:** Custom software graphics pipeline to eliminate terminal screen flickering.
* **Asynchronous Input Abstraction:** Non-blocking hardware polling (using 'termios.h' on macOS/Linux and 'conio.h' on Windows).
* **Dynamic Difficulty Scaling:** Generative codeword adaptation algorithm based on performance matrix scores.

## Architecture & Build Status
* **Language Standard:** ISO/IEC C99
* **Build System:** CMake (Version 3.10+)
* **Target Platforms:** macOS, Linux, Microsoft Windows

## Installation & Compilation

To compile the native binary, clone the repository and execute the cross-platform toolchain build pipeline:

```bash
git clone [https://github.com/mirana5656/typing-bomb-game.git](https://github.com/mirana5656/typing-bomb-game.git)
cd typing-bomb-game
mkdir build && cd build
cmake ..
cmake --build .
