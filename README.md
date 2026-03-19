# Renderer

A barebones OpenGL renderer written in C++ by Anthony Chimbay. This is the foundation of a larger independent study project exploring 3D world reconstruction from multi-camera imagery, started at Lawrence University in January 2026.

## Requirements

- CMake 4.3.0+
- MinGW-w64 15.2.0+ (Windows) or Clang (macOS)
- Git (for FetchContent to download dependencies)
- Internet connection on first build

## Dependencies

All dependencies are fetched automatically via CMake FetchContent — no manual installation required.

- [GLFW 3.4](https://github.com/glfw/glfw) — windowing and input

## Building

### Windows (MinGW)

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### Running

```bash
.\build\renderer.exe
```

## Project Structure

```
project/
├── src/
│   └── main.cpp        # Entry point and render loop
├── CMakeLists.txt      # Build configuration
├── .clangd             # Clangd language server config
├── .zed/
│   └── settings.json   # Zed editor settings
└── build/              # Generated build files (not committed)
```

## What It Does Currently

- Initializes a GLFW window (800x600)
- Sets up an OpenGL context
- Runs a render loop clearing the screen to dark grey
- Closes cleanly on window close

## Roadmap

- [ ] Colored triangle (first GLSL shaders)
- [ ] Flyable camera with keyboard/mouse input
- [ ] Colored cube
- [ ] PLY point cloud loader
- [ ] Gaussian Splatting renderer
- [ ] Multi-camera reconstruction pipeline

## Platform Notes

**Windows:** Built and tested with MinGW-w64 15.2.0 and CMake 4.3.0 on a PC with AMD RX 7900 XTX.

**macOS:** Targeting MacBook Air M2 (2022). Will require MoltenVK for Vulkan support in later stages.

## Copyright

Copyright (c) 2026 Anthony Chimbay. All rights reserved.

This project and its source code are proprietary. No part of this project may be reproduced, distributed, or transmitted in any form without prior written permission from the author.
