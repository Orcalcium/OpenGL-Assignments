#include "slime_character.h"
#include "obj_loader.h"
#include "../include/glad/glad.h"
#include "shader_code_loader.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>
#include <ctime>
#include <cmath>

#include "../include/stb/stb_image.h"

SlimeCharacter::SlimeCharacter() 
    : m_position(0.0f), m_rotation(0.0f), m_scale(1.0f), m_speed(2.0f),
      m_currentDirection(1.0f, 0.0f, 0.0f), m_targetDirection(1.0f, 0.0f, 0.0f),
      m_directionChangeTimer(0.0f), m_directionChangeInterval(2.0f), 
      m_directionBlendSpeed(2.0f), m_movementBounds(50.0f),
      m_VAO(0), m_VBO(0), m_EBO(0), m_texture(0), m_shaderProgram(0) {
}

SlimeCharacter::~SlimeCharacter() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
    if (m_texture) glDeleteTextures(1, &m_texture);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
}

bool SlimeCharacter::initialize() {
    if (!loadMesh()) {
        std::cerr << "Failed to load slime mesh" << std::endl;
        return false;
    }
    
    if (!loadTexture()) {
        std::cerr << "Failed to load slime texture" << std::endl;
        return false;
    }
    
    m_shaderProgram = createShaderProgram();
    if (!m_shaderProgram) {
        std::cerr << "Failed to create slime shader program" << std::endl;
        return false;
    }
    
    glm::vec3 initialDirection = generateRandomDirection();
    m_currentDirection = initialDirection;
    m_targetDirection = initialDirection;
    
    return true;
}

bool SlimeCharacter::loadMesh() {
    Mesh mesh;
    if (!SimpleOBJLoader::loadOBJ("assets/models/foliages/slime.obj", mesh)) {
        return false;
    }
    
    std::vector<float> vertices;
    for (const auto& vertex : mesh.vertices) {
        vertices.push_back(vertex.position.x);
        vertices.push_back(vertex.position.y);
        vertices.push_back(vertex.position.z);
        vertices.push_back(vertex.normal.x);
        vertices.push_back(vertex.normal.y);
        vertices.push_back(vertex.normal.z);
        vertices.push_back(vertex.texCoords.x);
        vertices.push_back(vertex.texCoords.y);
    }
    
    m_indices = mesh.indices;
    
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    return true;
}

bool SlimeCharacter::loadTexture() {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("assets/textures/slime_albedo.jpg", &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load slime texture" << std::endl;
        return false;
    }
    
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    return true;
}

unsigned int SlimeCharacter::createShaderProgram() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const std::string slimeVertexShaderCode = ShaderCodeLoader::loadShaderCode("shaders/slime.vert");
    const char* slimeVertexShader = slimeVertexShaderCode.c_str();
    glShaderSource(vertexShader, 1, &slimeVertexShader, nullptr);
    glCompileShader(vertexShader);
    
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Slime vertex shader compilation failed: " << infoLog << std::endl;
        return 0;
    }

    const std::string slimeFragmentShaderCode = ShaderCodeLoader::loadShaderCode("shaders/slime.frag");
    const char* slimeFragmentShader = slimeFragmentShaderCode.c_str();
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &slimeFragmentShader, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Slime fragment shader compilation failed: " << infoLog << std::endl;
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Slime shader program linking failed: " << infoLog << std::endl;
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}


void SlimeCharacter::update(float deltaTime) {
    m_directionChangeTimer += deltaTime;
    
    // Change target direction randomly every few seconds
    if (m_directionChangeTimer >= m_directionChangeInterval) {
        m_targetDirection = generateRandomDirection();
        m_directionChangeTimer = 0.0f;

        float minInterval = 1.5f;
        float maxInterval = 3.5f;
        float randomValue = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        m_directionChangeInterval = minInterval + (maxInterval - minInterval) * randomValue;
    }
    
    float blendFactor = glm::min(1.0f, m_directionBlendSpeed * deltaTime);
    m_currentDirection = glm::mix(m_currentDirection, m_targetDirection, blendFactor);
    m_currentDirection = glm::normalize(m_currentDirection);
    
    glm::vec3 nextPosition = m_position + m_currentDirection * m_speed * deltaTime;
    
    // Check bounds and adjust target direction if needed
    if (glm::length(nextPosition) > m_movementBounds) {
        glm::vec3 toCenter = -glm::normalize(m_position);
        m_targetDirection = glm::normalize(toCenter + generateRandomDirection() * 0.3f);
        m_directionChangeTimer = 0.0f;
    } else {
        m_position = nextPosition;
    }
    
    // Update rotation
    if (glm::length(m_currentDirection) > 0.001f) {
        m_rotation.y = atan2(m_currentDirection.x, m_currentDirection.z);
    }
}

glm::vec3 SlimeCharacter::generateRandomDirection() {
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * glm::pi<float>();
    return glm::vec3(sin(angle), 0.0f, cos(angle)); // Keep Y=0 for ground movement
}

void SlimeCharacter::render(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(m_shaderProgram);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_position);
    model = glm::rotate(model, m_rotation.y, glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(m_scale));
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightDir"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "slimeTexture"), 0);
    
    // Render
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
}
