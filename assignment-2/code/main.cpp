#include "../include/glad/glad.h"
#include "../include/GLFW/install/include/GLFW/glfw3.h"

#include "../include/glm/glm/gtc/matrix_transform.hpp"
#include "../include/glm/glm/gtc/type_ptr.hpp"

#include "../include/imgui/imgui.h"
#include "../include/imgui/backends/imgui_impl_glfw.h"
#include "../include/imgui/backends/imgui_impl_opengl3.h"

#include "shader.h"
#include "camera.h"
#include "model.h"
#include <iostream>

// Post-processing options
enum class PostProcessEffect {
    None = 0,
    Abstraction,
    Watercolor,
    Magnifier,
    Bloom,
    Pixelization,
    SineWaveDistortion,
};

enum class ViewMode {
    Scene = 0,
    Normal,
    PostProcessing,
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void setupFrameBuffer();
void renderPostProcessing();

GLuint createNoiseTexture(int width, int height);

// default configuration for screen size
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 900;

// Cursor control for ImGui/camera
bool cursorVisible = false;
bool altPressedLastFrame = false;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Mouse position for magnifier
double mouseX = 0.0;
double mouseY = 0.0;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float globalTime = 0.0f;

// Framebuffer
GLuint framebuffer;
GLuint colorTexture;
GLuint normalTexture;
GLuint noiseTexture;
GLuint rbo;

// Shader programs
Shader shader_scene;
Shader shader_postProcessing;

// Magnifier parameters
float magnifierRadius = 0.1f; // Radius of the magnifier effect
float magnifierStrength = 1.5f; // Strength of the magnification effect
glm::vec2 magnifierCenter = glm::vec2(0.0f, 0.0f); // Center of the magnifier effect

// Comparison slider
bool isDraggingSlider = false;
bool isDraggingScene = false;
float sliderPosition = 0.5f; 
float sliderWidth = 0.01f;         
float sliderLineWidth = 0.002f;    
float edgeThreshold = 0.06f;
float pixelizationScale = 0.01f;
glm::vec3 sliderColor = glm::vec3(0.0f, 0.0f, 0.0f);  

PostProcessEffect currentEffect = PostProcessEffect::None;
ViewMode currentView = ViewMode::Scene;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    //global OpenGL state
    glEnable(GL_DEPTH_TEST);

    //Build and compile shaders
    shader_scene = Shader("shaders/scene.vert", "shaders/scene.frag");
    shader_postProcessing = Shader("shaders/post_processing.vert", "shaders/post_processing.frag");


    Model model("lost-empire/lost_empire.obj");

    setupFrameBuffer();

    noiseTexture = createNoiseTexture(SCR_WIDTH, SCR_HEIGHT);

    while (!glfwWindowShouldClose(window))
    {
        
        float currentFrame = glfwGetTime();
        globalTime = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        
        processInput(window);
                
        // declare ImGui frame layout
        // dont render ImGui before 
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        
        ImGui::NewFrame();
        ImGui::Begin("Post-Processing Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SeparatorText("Mouse Position");
        ImGui::Text("Mouse X: %.1f", mouseX);
        ImGui::Text("Mouse Y: %.1f", mouseY);
        const char* viewmode[] = {
            "Scene",
            "Normal",
            "PostProcessing"
        };
        int viewIdx = static_cast<int>(currentView);
        ImGui::SeparatorText("View mode");
        if (ImGui::Combo("View", &viewIdx, viewmode, IM_ARRAYSIZE(viewmode))) {
            currentView = static_cast<ViewMode>(viewIdx);
        }

        // Post-processing effect selection
        
        
        if (currentView == ViewMode::PostProcessing) {
            ImGui::SeparatorText("Post-Processing Effects");
            const char* effectNames[] = {
                "None",
                "Abstraction",
                "Watercolor",
                "Magnifier",
                "Bloom",
                "Pixelization",
                "SineWaveDistortion"
            };
            int effectIdx = static_cast<int>(currentEffect);
            if (ImGui::Combo("Effect", &effectIdx, effectNames, IM_ARRAYSIZE(effectNames))) {
                currentEffect = static_cast<PostProcessEffect>(effectIdx);
            }
            if(currentEffect==PostProcessEffect::Abstraction)
            {
                ImGui::SliderFloat("Edge Threshold", &edgeThreshold, 0.0f, 0.25f, "%.2f");
            }
            if(currentEffect==PostProcessEffect::Pixelization)
            {
                ImGui::SliderFloat("Pixelization Scale", &pixelizationScale, 0.01f, 0.5f, "%.3f");
            }
            if(currentEffect==PostProcessEffect::Magnifier)
            {
                ImGui::SliderFloat("Magnifier Strength", &magnifierStrength, 1.0f, 2.0f, "%.2f");
            }
            ImGui::SeparatorText("Slider Appearance");
            ImGui::SliderFloat("Slider Width", &sliderLineWidth, 0.001f, 0.02f);
            ImGui::ColorEdit3("Slider Color", &sliderColor.x);

        }
        ImGui::End();
        
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render scene
        shader_scene.use();
        glm::mat4 transform_projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 transform_view = camera.GetViewMatrix();
        shader_scene.setMat4("projection", transform_projection);
        shader_scene.setMat4("view", transform_view);

        // Render the loaded model
        glm::mat4 tranfrom_model = glm::mat4(1.0f);
        tranfrom_model = glm::translate(tranfrom_model, glm::vec3(0.0f, -1.75f, 0.0f));
        tranfrom_model = glm::scale(tranfrom_model, glm::vec3(0.2f));
        shader_scene.setMat4("model", tranfrom_model);

        if( currentView == ViewMode::Normal ) {
            shader_scene.setInt("viewMode", 1);
        } 
        if(currentView == ViewMode::Scene) {
            shader_scene.setInt("viewMode", 0);
        }
        if(currentView == ViewMode::PostProcessing) {
            shader_scene.setInt("viewMode", 2);
        }
        model.Draw(shader_scene);
        //switch to default framebuffer(GLFW window)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST); // disable depth testing so quad isn't discarded due to depth test
        glClear(GL_COLOR_BUFFER_BIT);
        shader_postProcessing.use();
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glActiveTexture(GL_TEXTURE0);

        // Set the slider position as uniform values
        shader_postProcessing.setFloat("sliderPosition", sliderPosition);
        shader_postProcessing.setFloat("sliderWidth", sliderWidth);
        shader_postProcessing.setFloat("sliderLineWidth", sliderLineWidth);
        shader_postProcessing.setVec3("sliderColor", sliderColor);
        shader_postProcessing.setInt("viewMode", static_cast<int>(currentView));
        shader_postProcessing.setFloat("edgeThreshold", edgeThreshold);
        shader_postProcessing.setFloat("pixelizationScale", pixelizationScale);
        shader_postProcessing.setInt("screenTexture", 0);
        shader_postProcessing.setInt("noiseTexture", 1);
        shader_postProcessing.setVec2("magnifierCenter", magnifierCenter);
        shader_postProcessing.setFloat("magnifierRadius", magnifierRadius);
        shader_postProcessing.setFloat("magnifierStrength", magnifierStrength);
        shader_postProcessing.setFloat("mouseX", static_cast<float>(mouseX) / SCR_WIDTH);
        shader_postProcessing.setFloat("mouseY", static_cast<float>(mouseY) / SCR_HEIGHT);
        shader_postProcessing.setFloat("time", globalTime);
        shader_postProcessing.setFloat("screenWidth", static_cast<float>(SCR_WIDTH));
        shader_postProcessing.setFloat("screenHeight", static_cast<float>(SCR_HEIGHT));


        // Pass the selected effect to the shader as an int
        shader_postProcessing.setInt("effectType", static_cast<int>(currentEffect));
        
        renderPostProcessing();

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

void setupFrameBuffer()
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create a color attachment texture
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // Create a renderbuffer object for depth and stencil attachment
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderPostProcessing()
{
    //after Model.Draw() we get a texture of the rendered scene
    //to render the post-processing effects, we draw a quad that covers the entire screen
    //and bind the colorTexture to it
    //apply post-processing shader on the quad and we will get the post-processed image to display
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX=xpos/SCR_WIDTH;
    mouseY=ypos/SCR_HEIGHT;
    magnifierCenter = glm::vec2(mouseX,1.0-mouseY);

    if (isDraggingSlider) {
        sliderPosition = xpos / SCR_WIDTH;
        sliderPosition = std::max(0.0f, std::min(1.0f, sliderPosition));
        return;
    }

    if (isDraggingScene) {
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

        camera.ProcessMouseMovement(xoffset, yoffset);
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
            if (abs(normalizedX - sliderPosition) < sliderWidth ) {
                isDraggingSlider = true;
            }
            
        } else if (action == GLFW_RELEASE) {
            isDraggingSlider = false;
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
    camera.ProcessMouseScroll(yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    
    if (framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &colorTexture);
        glDeleteTextures(1, &normalTexture);
        glDeleteRenderbuffers(1, &rbo);

        setupFrameBuffer();
    }
    glViewport(0, 0, width, height);

    // Update noise texture size
    if (noiseTexture) {
        glDeleteTextures(1, &noiseTexture);
        noiseTexture = createNoiseTexture(width, height);
    }
}

GLuint createNoiseTexture(int width , int height) {
    int texWidth = width / 3;
    int texHeight = height / 3;
    std::vector<unsigned char> noiseData(texWidth * texHeight * 3);
    for (int i = 0; i < texWidth * texHeight * 3; ++i)
        noiseData[i] = rand() % 256;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, noiseData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return textureID;
}