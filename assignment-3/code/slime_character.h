#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class SlimeCharacter {
public:
    SlimeCharacter();
    ~SlimeCharacter();
    
    bool initialize();
    void update(float deltaTime);
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    glm::vec3 getPosition() const { return m_position; }
    
private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    float m_scale;
    float m_speed;
    
    // Random walk variables
    glm::vec3 m_currentDirection;
    glm::vec3 m_targetDirection;
    float m_directionChangeTimer;
    float m_directionChangeInterval;
    float m_directionBlendSpeed;
    float m_movementBounds;
    
    // Rendering
    unsigned int m_VAO, m_VBO, m_EBO;
    unsigned int m_texture;
    unsigned int m_shaderProgram;
    std::vector<unsigned int> m_indices;
    
    bool loadMesh();
    bool loadTexture();
    unsigned int createShaderProgram();
    glm::vec3 interpolatePosition(float t);
    glm::vec3 generateRandomDirection();
};
