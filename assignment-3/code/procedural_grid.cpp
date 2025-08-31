#include "procedural_grid.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "shader_code_loader.h"


ProceduralGrid::ProceduralGrid() : m_VAO(0), m_VBO(0), m_shaderProgram(0) {
}

ProceduralGrid::~ProceduralGrid() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
}

bool ProceduralGrid::initialize() {
    // Create a large quad for the ground
    float vertices[] = {
        // positions
        -500.0f, 0.0f, -500.0f,
         500.0f, 0.0f, -500.0f,
         500.0f, 0.0f,  500.0f,
        -500.0f, 0.0f,  500.0f,
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    m_shaderProgram = createShaderProgram();
    if (!m_shaderProgram) {
        std::cerr << "Failed to create grid shader program" << std::endl;
        return false;
    }
    
    return true;
}

unsigned int ProceduralGrid::createShaderProgram() {
    std::string vertSource = ShaderCodeLoader::loadShaderCode("shaders/grid.vert");
    std::string fragSource = ShaderCodeLoader::loadShaderCode("shaders/grid.frag");

    if (vertSource.empty() || fragSource.empty()) {
        std::cerr << "Failed to load grid shader sources." << std::endl;
        return 0;
    }

    const char* vSrc = vertSource.c_str();
    const char* fSrc = fragSource.c_str();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vSrc, nullptr);
    glCompileShader(vertexShader);
    
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(vertexShader, 1024, nullptr, infoLog);
        std::cerr << "Grid vertex shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return 0;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fSrc, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(fragmentShader, 1024, nullptr, infoLog);
        std::cerr << "Grid fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        std::cerr << "Grid shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void ProceduralGrid::render(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(m_shaderProgram);
    
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
