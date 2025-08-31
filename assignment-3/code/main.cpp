#include "../include/glad/glad.h"
#include "../include/GLFW/install/include/GLFW/glfw3.h"

#include "../include/glm/glm/gtc/matrix_transform.hpp"
#include "../include/glm/glm/gtc/type_ptr.hpp"

#include "../include/imgui/imgui.h"
#include "../include/imgui/backends/imgui_impl_glfw.h"
#include "../include/imgui/backends/imgui_impl_opengl3.h"

#include "camera.h"
#include "foliage_renderer.h"
#include "slime_character.h"
#include "procedural_grid.h"
#include <iostream>
#include <chrono>

enum class CameraMode {
    God = 0,
    Player
};

enum class PostProcessEffect {
    None = 0,
    Abstraction,
    Watercolor,
    Magnifier,
    Bloom,
    Pixelization,
    SineWaveDistortion,
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


// default configuration for screen size
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 900;

// Cursor control for ImGui
bool cursorVisible = false;
bool altPressedLastFrame = false;
float mouseX, mouseY;
bool isDraggingScene = false;

Camera godCamera(glm::vec3(0.0f, 20.0f, 30.0f));
Camera playerCamera(glm::vec3(0.0f, 7.0f, 5.0f));
Camera* currentCamera = &godCamera;
CameraMode cameraMode = CameraMode::God;

// Player movement
float playerYaw = -90.0f;
float playerPitch = 0.0f;
glm::vec3 playerPosition(0.0f, 7.0f, 5.0f);
glm::vec3 playerFront(0.0f, 0.0f, -1.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float globalTime = 0.0f;

// Scene objects
FoliageRenderer foliageRenderer;
SlimeCharacter slimeCharacter;
ProceduralGrid proceduralGrid;

int currentSampleSet = 0;
const std::vector<std::string> sampleFiles = {
    "assets/models/spatialSamples/poissonPoints_1010s.ss2",
    "assets/models/spatialSamples/poissonPoints_2797s.ss2",
    "assets/models/spatialSamples/poissonPoints_155304s.ss2"
};

float frameCount = 0;
float fps = 0;
float fpsTimer = 0;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 3 - Indirect Rendering", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!foliageRenderer.initialize()) {
        std::cerr << "Failed to initialize foliage renderer" << std::endl;
        return -1;
    }
    
    if (!slimeCharacter.initialize()) {
        std::cerr << "Failed to initialize slime character" << std::endl;
        return -1;
    }
    
    if (!proceduralGrid.initialize()) {
        std::cerr << "Failed to initialize procedural grid" << std::endl;
        return -1;
    }

    foliageRenderer.loadPoissonSamples(sampleFiles[currentSampleSet]);

    auto frameStartCPU = std::chrono::high_resolution_clock::now();
    double lastFrameTotalMs = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        globalTime = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            fps = frameCount / fpsTimer;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        processInput(window);
        
        slimeCharacter.update(deltaTime);
        
        foliageRenderer.checkCollisions(slimeCharacter.getPosition(), 1.0f);
                
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        
        ImGui::NewFrame();
        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.1f", fps);
        ImGui::SeparatorText("Camera Mode");
        const char* cameraModes[] = {"God View", "Player View"};
        int camIdx = static_cast<int>(cameraMode);
        if (ImGui::Combo("Camera", &camIdx, cameraModes, IM_ARRAYSIZE(cameraModes))) {
            cameraMode = static_cast<CameraMode>(camIdx);
            currentCamera = (cameraMode == CameraMode::God) ? &godCamera : &playerCamera;
        }
        
        ImGui::SeparatorText("Spatial Samples");
        const char* sampleNames[] = {"1,010 samples", "2,797 samples", "155,304 samples"};
        int prevSample = currentSampleSet;
        if (ImGui::Combo("Sample Set", &currentSampleSet, sampleNames, IM_ARRAYSIZE(sampleNames))) {
            if (currentSampleSet != prevSample) {
                foliageRenderer.loadPoissonSamples(sampleFiles[currentSampleSet]);
            }
        }

        ImGui::SeparatorText("Player Controls");
        ImGui::Text("Player View: W/S (forward/back), A/D (turn)");
        ImGui::Text("God View: Free camera with mouse + WASD");
        ImGui::Text("Slime Position: (%.1f, %.1f, %.1f)", 
                   slimeCharacter.getPosition().x, 
                   slimeCharacter.getPosition().y, 
                   slimeCharacter.getPosition().z);

        ImGui::SeparatorText("Mouse Position");
        ImGui::Text("Mouse X: %.1f", mouseX);
        ImGui::Text("Mouse Y: %.1f", mouseY);

        // Foliage profiling
        const auto& prof = foliageRenderer.getProfileData();
        ImGui::SeparatorText("Frame Profiling (CPU ms)");

        // Measure frame CPU end here (previous frame total displayed this frame)
        ImGui::Text("Last Frame Total: %.3f", lastFrameTotalMs);
        double foliageSum = prof.cpuCullMs + prof.cpuSetupMs + prof.cpuDrawMs;

        ImGui::Text("Foliage Total: %.3f", foliageSum);
        if(foliageSum > 0.0) {
            ImGui::Text("  Cull:   %.3f (%.0f%%)", prof.cpuCullMs, (prof.cpuCullMs/foliageSum)*100.0);
            ImGui::Text("  Setup:  %.3f (%.0f%%)", prof.cpuSetupMs, (prof.cpuSetupMs/foliageSum)*100.0);
            ImGui::Text("  Draw:   %.3f (%.0f%%)", prof.cpuDrawMs, (prof.cpuDrawMs/foliageSum)*100.0);
        }

        double foliageDetail = prof.cpuBuildSourceMs + prof.cpuDispatchMs + prof.cpuReadbackMs + prof.cpuReorderMs;
        if(foliageDetail > 0.0) {
            ImGui::SeparatorText("Foliage Detailed");
            ImGui::Text("  BuildSource: %.3f", prof.cpuBuildSourceMs);
            ImGui::Text("  Dispatch:    %.3f", prof.cpuDispatchMs);
            ImGui::Text("  Readback:    %.3f", prof.cpuReadbackMs);
            ImGui::Text("  Reorder:     %.3f", prof.cpuReorderMs);
        }

        double otherCPU = lastFrameTotalMs - foliageSum;
        if(lastFrameTotalMs > 0.0) ImGui::Text("Other CPU: %.3f", otherCPU);
        ImGui::Separator();
        ImGui::SeparatorText("Collision Profiling");
        ImGui::Text("Loop: %.3f ms", prof.cpuCollisionLoopMs);
        ImGui::Text("Rebuild: %.3f ms", prof.cpuCollisionRebuildMs);
        ImGui::Text("Tests: %u  Hits: %u", prof.collisionTests, prof.collisionHits);
        ImGui::End();

        // Update lastFrameTotalMs at end of UI build (using previous frameStartCPU)
        auto frameEndCPU = std::chrono::high_resolution_clock::now();
        lastFrameTotalMs = std::chrono::duration<double, std::milli>(frameEndCPU - frameStartCPU).count();
        frameStartCPU = frameEndCPU;

        // Render god and player views
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update camera based on mode
        if (cameraMode == CameraMode::Player) {
            playerCamera.Position = playerPosition;
            playerCamera.Front = playerFront;
        }

        // Matrices for both cameras
        glm::mat4 godView = godCamera.GetViewMatrix();
        glm::mat4 playerView = playerCamera.GetViewMatrix();
        float aspectHalf = (SCR_WIDTH * 0.5f) / (float)SCR_HEIGHT;
        glm::mat4 godProjection = glm::perspective(glm::radians(godCamera.Zoom), aspectHalf, 0.1f, 1000.0f);
        
        //make playerProjection's range smaller to display the culling mechanism
        glm::mat4 playerProjection = glm::perspective(glm::radians(playerCamera.Zoom), aspectHalf, 0.1f, 100.0f);

        // God view
        glViewport(0, 0, SCR_WIDTH/2, SCR_HEIGHT);
        proceduralGrid.render(godView, godProjection);
        foliageRenderer.render(godView, godProjection, godCamera.Position,
                    playerView, playerProjection, playerCamera.Position);
        slimeCharacter.render(godView, godProjection);

        // Player view
        glViewport(SCR_WIDTH/2, 0, SCR_WIDTH/2, SCR_HEIGHT);
        proceduralGrid.render(playerView, playerProjection);
        foliageRenderer.render(playerView, playerProjection, playerCamera.Position,
                    playerView, playerProjection, playerCamera.Position);
        slimeCharacter.render(playerView, playerProjection);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (cameraMode == CameraMode::God) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            godCamera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            godCamera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            godCamera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            godCamera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            godCamera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            godCamera.ProcessKeyboard(DOWN, deltaTime);
    } else {
        float velocity = 10.0f * deltaTime;
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            playerPosition += playerFront * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            playerPosition -= playerFront * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            playerYaw -= 45.0f * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            playerYaw += 45.0f * deltaTime;
        }
        
        // Update player front vector
        glm::vec3 front;
        front.x = cos(glm::radians(playerYaw)) * cos(glm::radians(playerPitch));
        front.y = sin(glm::radians(playerPitch));
        front.z = sin(glm::radians(playerYaw)) * cos(glm::radians(playerPitch));
        playerFront = glm::normalize(front);
        
    playerPosition.y = 7.0f;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX=xpos/SCR_WIDTH;
    mouseY=ypos/SCR_HEIGHT;

    if (isDraggingScene && cameraMode == CameraMode::God) {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        godCamera.ProcessMouseMovement(xoffset, yoffset);
    }
    
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            float normalizedX = xpos / SCR_WIDTH;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT&&action==GLFW_PRESS){
        isDraggingScene=true;
    }
    else{
        isDraggingScene=false;
        firstMouse=true;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (cameraMode == CameraMode::God) {
        godCamera.ProcessMouseScroll(yoffset);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    
    glViewport(0, 0, width, height);

}
