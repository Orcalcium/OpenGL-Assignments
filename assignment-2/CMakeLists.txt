cmake_minimum_required(VERSION 3.10)
project(project)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add cpp file to the project
add_executable(project 
    ./code/main.cpp
    ./include/glad/glad.c
    ./include/imgui/imgui.cpp
    ./include/imgui/imgui_draw.cpp
    ./include/imgui/imgui_tables.cpp
    ./include/imgui/imgui_widgets.cpp
    ./include/imgui/backends/imgui_impl_glfw.cpp
    ./include/imgui/backends/imgui_impl_opengl3.cpp
    ./code/stb_image.cpp
)

# Specify the path to the installed GLFW library
set(GLFW_DIR "./include/GLFW/install/lib/cmake/glfw3")
set(ASSIMP_DIR "./include/assimp/install/lib/cmake/assimp-5.4")

# Find the GLFW package 
find_package(glfw3 REQUIRED PATHS ${GLFW_DIR})
find_package(assimp REQUIRED PATHS ${ASSIMP_DIR})

# Set CMP0072 explicitly to use modern OpenGL (GLVND) implementation
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE "GLVND")

# Find the OpenGL package
find_package(OpenGL REQUIRED)

# Include directories for GLAD, GLFW, Assimp, and other libraries
target_include_directories(project PRIVATE 
    ./include/glad
    ./include/GLFW
    ./include/glm
    ./include/imgui
    ./include/imgui/backends
    ./include/KHR
    ./include/assimp/install/include  # Ensure this path is correct
)

target_link_libraries(project PRIVATE glfw assimp::assimp OpenGL::GL)