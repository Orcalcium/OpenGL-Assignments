# Assignment 2: Framebuffer Processing and Post Effect

## Overview
This assignment explores advanced OpenGL techniques by implementing framebuffer-based post-processing effects. The project demonstrates how to import large-scale 3D models, render them efficiently, and apply visual effects using shaders and framebuffers.

## Features
- OBJ model loading using Assimp
- Scene rendering with OpenGL
- Framebuffer setup for off-screen rendering
- Post-processing effects (e.g., color correction, blur, custom shader effects)

## Prerequisites
- C++ compiler (GCC/Clang/MSVC)
- CMake
- OpenGL 3.3+ compatible GPU
- Assimp library (included in `include/`)
- GLFW, GLAD, GLM, stb, ImGui (included in `include/`)

## Building & Running
1. Open a terminal and navigate to the assignment directory:
   ```bash
   cd assignment-2
   ```
2. Create a build directory and compile the project:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. Run the executable:
   ```bash
   ./project
   ```

## Project Structure
```
OpenGL-Assignments/
└── assignment-2/
    ├── code/
    │   ├── main.cpp
    │   ├── camera.h
    │   ├── model.h
    │   ├── shader.h
    │   └── stb_image.cpp
    ├── include/
    ├── lost-empire/
    │   ├── lost_empire.obj
    │   ├── lost_empire.mtl
    │   ├── lost_empire.jpg
    │   ├── lost_empire-Alpha.png
    │   ├── lost_empire-RGB.png
    │   └── lost_empire-RGBA.png
    ├── shaders/
    │   ├── scene.vert
    │   ├── scene.frag
    │   ├── post_processing.vert
    │   └── post_processing.frag
    ├── CMakeLists.txt
    └── README.md
```

## Implementation
The scene is first rendered to a framebuffer object (FBO) instead of directly to the screen. The contents of the FBO are then processed using fragment shaders to apply effects such as color grading, blurring, or custom visual styles. The final image is rendered to the screen from the processed framebuffer.

## Controls
- Use keyboard/mouse to move the trackball and interact with the scene.
- Use Imgui menu to configure views and postprocessing effects

## Screenshots
![Postprocessing Example](showcase.webp)

## Credits
- Libraries: Assimp, GLFW, GLAD, GLM, stb, ImGui
- Course: CS 550700 Introduction to Graphics Programming and Applications, NTHU

For more details, refer to the code and comments in the source files.
