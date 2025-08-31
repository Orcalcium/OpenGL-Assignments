#define STB_IMAGE_IMPLEMENTATION
#include "../../include/stb/stb_image.h"

// imgui header files
#include "../include/imgui/imgui.h"
#include "../include/imgui/backends/imgui_impl_glfw.h"
#include "../include/imgui/backends/imgui_impl_opengl3.h"

//header files from code directory
#include "./fileloader/fileLoader.h"
#include "./mesh/mesh.h"
#include "./robot/robot.h"

//glad and glfw header files
#include "../include/glad/glad.h"
#include "../include/GLFW/install/include/GLFW/glfw3.h"
//glm header 

#include "../include/glm/glm/glm.hpp"
#include "../include/glm/glm/gtc/matrix_transform.hpp"
#include "../include/glm/glm/gtc/type_ptr.hpp"

#include <iostream>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
unsigned int buildSceneShaderProgram();
unsigned int buildCrosshairShaderProgram();
void sendCursorVertData(unsigned int &VBO, unsigned int &VAO, float* vertices, int vertCount);
void sendCubeVertData(unsigned int &VBO, unsigned int &VAO, float* vertices, int vertCount);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void drawCrosshair(unsigned int shaderProgram, unsigned int VAO, int vertCount);
void drawScene(unsigned int shaderProgram, unsigned int VAO, int vertCount);

//Variabled to store window size
float WINDOW_WIDTH = 800.0f;
float WINDOW_HEIGHT = 600.0f;

// Variables to store rotation angles and last mouse position
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;
FileLoader fileLoader;

int viewmode = 0;

// Add global camera variables
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Get the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor)
    {
        std::cout << "Failed to get primary monitor" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Get the video mode of the primary monitor
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    if (!mode)
    {
        std::cout << "Failed to get video mode" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create a windowed mode window with the same resolution as the primary monitor
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Project", primaryMonitor, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Build and compile scene shader program
    unsigned int sceneShaderProgram = buildSceneShaderProgram();
    unsigned int crosshairShaderProgram = buildCrosshairShaderProgram();
    Cube cube;
    // Set up vertex data (and buffer(s)) and configure vertex attributes

    unsigned int cubeVBO, cubeVAO;
    sendCubeVertData(cubeVBO, cubeVAO, cube.getMesh(), cube.getVertCount());

    Crosshair crosshair;
    unsigned int crosshairVBO, crosshairVAO;
    sendCursorVertData(crosshairVBO, crosshairVAO, crosshair.getMesh(), crosshair.getVertCount());

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f);
    // Render loop
    Robot robot;
    bool rotate = false;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Toggle rotation with a key press (e.g., 'R')
        static int released = 1;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
            released = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            if (released) {
                released = 0;
                rotate = !rotate;
                robot.rotate = rotate;
            }
        }

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create ImGui window
        ImGui::Begin("Controls");
        ImGui::Text("WASD: Move");
        ImGui::Text("R: Rotate");
        ImGui::Text("Space: Move Up");
        ImGui::Text("Control: Move Down");
        ImGui::End();

        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        robot.draw(sceneShaderProgram, cubeVAO, cameraPos, cameraFront, cameraUp, WINDOW_WIDTH, WINDOW_HEIGHT);

        // Draw the cursor
        drawCrosshair(crosshairShaderProgram, crosshairVAO, crosshair.getVertCount());

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Deallocate resources
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteProgram(sceneShaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Add camera translation:
    const float cameraSpeed = 0.05f; // adjust speed as needed
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos.y -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos.y += cameraSpeed;

    // Toggle cursor visibility when Shift is pressed
    /*static bool shiftPressed = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
    {
        glfwSetCursorPos(window, WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
        firstMouse = true;
        if (!shiftPressed)
        {
            shiftPressed = true;
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                viewmode = 1;
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                viewmode = 0;
            }
        }
    }
    else
    {
        shiftPressed = false;
    }
        */
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        xoffset = 0;
        yoffset = 0;
    }

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch angle
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Update cameraFront vector using updated yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    if (viewmode == 0) cameraFront = glm::normalize(front);
}

unsigned int buildSceneShaderProgram() {
    // load the shader files
    std::string vertShaderStr = fileLoader.loadFile("./code/shader/scene/vert.glsl");
    std::string fragShaderStr = fileLoader.loadFile("./code/shader/scene/frag.glsl");
    const char* vertShaderSource = vertShaderStr.c_str();
    const char* fragShaderSource = fragShaderStr.c_str();

    // Build and compile our shader program
    // Vertex shader
    unsigned int sceneVertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sceneVertShader, 1, &vertShaderSource, NULL);
    glCompileShader(sceneVertShader);
    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(sceneVertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(sceneVertShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Fragment shader
    unsigned int sceneFragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sceneFragShader, 1, &fragShaderSource, NULL);
    glCompileShader(sceneFragShader);
    // Check for shader compile errors
    glGetShaderiv(sceneFragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(sceneFragShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, sceneVertShader);
    glAttachShader(shaderProgram, sceneFragShader);
    glLinkProgram(shaderProgram);
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(sceneVertShader);
    glDeleteShader(sceneFragShader);
    return shaderProgram;
}

unsigned int buildCrosshairShaderProgram() {
    // load the shader files
    std::string vertShaderStr = fileLoader.loadFile("./code/shader/crosshair/vert.glsl");
    std::string fragShaderStr = fileLoader.loadFile("./code/shader/crosshair/frag.glsl");
    const char* vertShaderSource = vertShaderStr.c_str();
    const char* fragShaderSource = fragShaderStr.c_str();

    // Build and compile our shader program
    // Vertex shader
    unsigned int sceneVertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sceneVertShader, 1, &vertShaderSource, NULL);
    glCompileShader(sceneVertShader);
    // Check shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(sceneVertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(sceneVertShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Fragment shader
    unsigned int sceneFragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sceneFragShader, 1, &fragShaderSource, NULL);
    glCompileShader(sceneFragShader);
    // Check shader compile errors
    glGetShaderiv(sceneFragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(sceneFragShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, sceneVertShader);
    glAttachShader(shaderProgram, sceneFragShader);
    glLinkProgram(shaderProgram);
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(sceneVertShader);
    glDeleteShader(sceneFragShader);
    return shaderProgram;
}

void sendCursorVertData(unsigned int &VBO, unsigned int &VAO, float* vertices, int vertCount) {
    // Set up vertex data (and buffer(s)) and configure vertex attributes
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertCount * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind the VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void sendCubeVertData(unsigned int &VBO, unsigned int &VAO, float* vertices, int vertCount){
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Use the actual data size: number of floats * sizeof(float)
    glBufferData(GL_ARRAY_BUFFER, vertCount * sizeof(float), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void drawScene(unsigned int shaderProgram, unsigned int VAO, int vertCount) {
    glUseProgram(shaderProgram);

    // Transformation matrices
    glm::mat4 model = glm::mat4(1.0f); // Keep model static
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieve uniform locations (unchanged)
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Pass matrices to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Render the cube (unchanged)
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// Function to draw a simple crosshair cursor in the middle of the window
void drawCrosshair(unsigned int shaderProgram, unsigned int VAO, int vertCount) {
    glUseProgram(shaderProgram);

    // Transformation matrices
    glm::mat4 projection = glm::ortho(-WINDOW_WIDTH / 2.0f, WINDOW_WIDTH / 2.0f, -WINDOW_HEIGHT / 2.0f, WINDOW_HEIGHT / 2.0f); // Orthographic projection centered at (0, 0); // Orthographic projection

    // Retrieve uniform locations
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Pass matrices to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Render the crosshair
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 4);
}