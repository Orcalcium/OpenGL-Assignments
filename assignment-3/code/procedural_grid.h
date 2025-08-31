#pragma once

#include "../include/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ProceduralGrid {
public:
    ProceduralGrid();
    ~ProceduralGrid();
    
    bool initialize();
    void render(const glm::mat4& view, const glm::mat4& projection);
    
private:
    unsigned int m_VAO, m_VBO;
    unsigned int m_shaderProgram;
    
    unsigned int createShaderProgram();
};
