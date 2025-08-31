#include "obj_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

bool SimpleOBJLoader::loadOBJ(const std::string& path, Mesh& mesh) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec2> temp_texCoords;
    std::vector<glm::vec3> temp_normals;
    
    std::vector<int> positionIndices, texCoordIndices, normalIndices;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            temp_positions.push_back(position);
        }
        else if (prefix == "vt") {
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            temp_texCoords.push_back(texCoord);
        }
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {
            std::string vertex1, vertex2, vertex3;
            iss >> vertex1 >> vertex2 >> vertex3;
            
            auto parseVertex = [&](const std::string& vertexStr) {
                std::istringstream vertexStream(vertexStr);
                std::string component;
                
                // Position index
                std::getline(vertexStream, component, '/');
                if (!component.empty()) {
                    positionIndices.push_back(std::stoi(component) - 1);
                }
                
                // Texture coordinate index
                std::getline(vertexStream, component, '/');
                if (!component.empty()) {
                    texCoordIndices.push_back(std::stoi(component) - 1);
                } else {
                    texCoordIndices.push_back(-1);
                }
                
                // Normal index
                std::getline(vertexStream, component);
                if (!component.empty()) {
                    normalIndices.push_back(std::stoi(component) - 1);
                } else {
                    normalIndices.push_back(-1);
                }
            };
            
            parseVertex(vertex1);
            parseVertex(vertex2);
            parseVertex(vertex3);
        }
    }
    
    // Process vertices and create final mesh
    processVertex(temp_positions, temp_texCoords, temp_normals,
                 positionIndices, texCoordIndices, normalIndices,
                 mesh.vertices, mesh.indices);
    
    return true;
}

void SimpleOBJLoader::processVertex(const std::vector<glm::vec3>& positions,
                                   const std::vector<glm::vec2>& texCoords,
                                   const std::vector<glm::vec3>& normals,
                                   const std::vector<int>& posIndices,
                                   const std::vector<int>& texIndices,
                                   const std::vector<int>& normalIndices,
                                   std::vector<Vertex>& vertices,
                                   std::vector<unsigned int>& indices) {
    
    std::unordered_map<std::string, unsigned int> uniqueVertices;
    
    for (size_t i = 0; i < posIndices.size(); i++) {
        Vertex vertex;
        
        // Position
        if (posIndices[i] >= 0 && posIndices[i] < positions.size()) {
            vertex.position = positions[posIndices[i]];
        }
        
        // Texture coordinates
        if (i < texIndices.size() && texIndices[i] >= 0 && texIndices[i] < texCoords.size()) {
            vertex.texCoords = texCoords[texIndices[i]];
        } else {
            vertex.texCoords = glm::vec2(0.0f);
        }
        
        // Normals
        if (i < normalIndices.size() && normalIndices[i] >= 0 && normalIndices[i] < normals.size()) {
            vertex.normal = normals[normalIndices[i]];
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default normal
        }
        
        // Create unique key for vertex
        std::string key = std::to_string(vertex.position.x) + "," +
                         std::to_string(vertex.position.y) + "," +
                         std::to_string(vertex.position.z) + "," +
                         std::to_string(vertex.texCoords.x) + "," +
                         std::to_string(vertex.texCoords.y) + "," +
                         std::to_string(vertex.normal.x) + "," +
                         std::to_string(vertex.normal.y) + "," +
                         std::to_string(vertex.normal.z);
        
        if (uniqueVertices.count(key) == 0) {
            uniqueVertices[key] = static_cast<unsigned int>(vertices.size());
            vertices.push_back(vertex);
        }
        
        indices.push_back(uniqueVertices[key]);
    }
}
