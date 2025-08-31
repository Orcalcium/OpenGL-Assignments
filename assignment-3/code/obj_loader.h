#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::string texturePath;
};

class SimpleOBJLoader {
public:
    static bool loadOBJ(const std::string& path, Mesh& mesh);
    
private:
    static void processVertex(const std::vector<glm::vec3>& positions,
                             const std::vector<glm::vec2>& texCoords,
                             const std::vector<glm::vec3>& normals,
                             const std::vector<int>& posIndices,
                             const std::vector<int>& texIndices,
                             const std::vector<int>& normalIndices,
                             std::vector<Vertex>& vertices,
                             std::vector<unsigned int>& indices);
};
