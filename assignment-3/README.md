# Assignment 3: GPU Driven Rendering with OpenGL

## Overview
This assignment demonstrates modern GPU-driven rendering techniques in OpenGL by efficiently rendering large-scale foliage using compute shaders and indirect drawing. The project covers advanced concepts such as frustum culling, procedural generation, and leveraging the GPU for scene management.

## Features
- GPU-driven rendering pipeline
- Compute shaders for culling and command generation
- Indirect drawing for efficient rendering of many objects
- Procedural grid and foliage generation
- Frustum culling based on camera viewpoint
- Modular code structure for easy extension

## Prerequisites
- C++ compiler (GCC/Clang/MSVC)
- CMake
- OpenGL 4.3+ compatible GPU (for compute shaders)
- GLFW, GLAD, GLM, stb, ImGui, Assimp (included in `include/`)

## Building & Running
1. Open a terminal and navigate to the assignment directory:
   ```bash
   cd assignment-3
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
└── assignment-3/
    ├── code/
    ├── include/
    ├── assets/
    │   ├── models/
    │   └── textures/
    ├── shaders/
    ├── CMakeLists.txt
    └── README.md
```

## Implementation
The project uses compute shaders to perform frustum culling and generate draw commands on the GPU. Indirect drawing allows the rendering large number scale foliage instances efficiently.

## Controls
- Use keyboard/mouse to move the trackball and interact with the scene.
- Use Imgui menu to configure views and postprocessing effects


## Screenshots
![Postprocessing Example](showcase.webp)

## Credits
- Libraries: GLFW, GLAD, GLM, stb, ImGui, Assimp
- Course: CS 550700 Introduction to Graphics Programming and Applications, NTHU

For more details, refer to the code and comments in the source files.
