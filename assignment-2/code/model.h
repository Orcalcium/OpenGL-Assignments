#ifndef MODEL_H
#define MODEL_H

#include "../include/glad/glad.h"
#include "../include/glm/glm/glm.hpp"
#include "../include/stb/stb_image.h"
#include "../include/assimp/install/include/assimp/Importer.hpp"
#include "../include/assimp/install/include/assimp/scene.h"
#include "../include/assimp/install/include/assimp/postprocess.h"

#include "shader.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        setupMesh();
    }

    void Draw(Shader &shader)
    {
        std::cout << "Drawing mesh with " << textures.size() << " textures" << std::endl;
        
        glActiveTexture(GL_TEXTURE0);
        
        if(textures.size() > 0) {
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            
            shader.setInt("texture1", 0);
            
            for (unsigned int i = 0; i < textures.size(); i++)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                
                std::string number;
                std::string name = textures[i].type;
                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if (name == "texture_specular")
                    number = std::to_string(specularNr++);

                std::cout << "Binding " << name << number << " (id: " << textures[i].id 
                    << ") to texture unit " << i << std::endl;
                
                shader.setInt((name + number).c_str(), i);
                
                if (i == 0) {
                    shader.setInt("texture1", 0); // Make sure texture1 is set
                }
                
                // Bind texture to current active texture unit
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
        } else {
            // If no textures, create a default color texture
            std::cout << "No textures in mesh - creating fallback texture" << std::endl;
            unsigned int defaultTexture = createDefaultTexture();
            shader.setInt("texture1", 0);
            glBindTexture(GL_TEXTURE_2D, defaultTexture);
        }

        // Debug VAO and vertex attribute state
        std::cout << "Drawing with VAO: " << VAO << std::endl;
        
        // Draw the mesh
        glBindVertexArray(VAO);
        
        // Additional debug info
        GLint currentVao;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
        std::cout << "Current VAO before draw: " << currentVao << std::endl;
        
        GLint isEnabled0, isEnabled1, isEnabled2;
        glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &isEnabled0);
        glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &isEnabled1);
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &isEnabled2);
        std::cout << "Attributes enabled: [0]=" << isEnabled0 << ", [1]=" << isEnabled1 
                  << ", [2]=" << isEnabled2 << std::endl;
        
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Reset to texture unit 0
        glActiveTexture(GL_TEXTURE0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // Vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // Vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
        

        // Debug prints
        std::cout << "VAO: " << VAO << ", VBO: " << VBO << ", EBO: " << EBO << std::endl;
    }
    
    unsigned int createDefaultTexture() {
        // Create a simple 2x2 colored texture as fallback
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Simple 2x2 green texture
        unsigned char data[] = {
            0, 255, 0, 255,   0, 255, 0, 255,
            0, 255, 0, 255,   0, 255, 0, 255
        };
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        return textureID;
    }
};

class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;

    Model(std::string const &path)
    {
        loadModel(path);
    }

    void Draw(Shader &shader)
    {
        // Print debug info for every mesh drawing
        std::cout << "Drawing model with " << meshes.size() << " meshes" << std::endl;
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    void loadModel(std::string const &path)
    {
        std::cout << "Loading model from: " << path << std::endl;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        std::cout << "Model loaded successfully. Directory: " << directory << std::endl;
        std::cout << "Nodes: " << scene->mRootNode->mNumChildren << ", Meshes: " << scene->mNumMeshes << std::endl;

        processNode(scene->mRootNode, scene);
        std::cout << "Model processing complete. Total meshes: " << meshes.size() << std::endl;
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
            
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        std::cout << "Processed mesh with " << vertices.size() << " vertices and " 
              << indices.size() << " indices" << std::endl;
    
        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

    bool fileExists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }
    
    unsigned int TextureFromFile(const char *path, const std::string &directory)
    {
        std::string filename = std::string(path);
        filename = directory + '/' + filename;
    
        std::cout << "Loading texture from: " << filename << std::endl;
    
        if (!fileExists(filename)) {
            std::cout << "File does not exist: " << filename << std::endl;
            
            // Try alternative paths
            std::string altPath = directory + "/textures/" + std::string(path);
            std::cout << "Trying alternative path: " << altPath << std::endl;
            if (fileExists(altPath)) {
                filename = altPath;
                std::cout << "Found texture at: " << filename << std::endl;
            } else {
                // Return a default texture if file not found
                std::cout << "Creating default texture instead" << std::endl;
                unsigned int defaultTexID = 0;
                glGenTextures(1, &defaultTexID);
                glBindTexture(GL_TEXTURE_2D, defaultTexID);
                
                // 1x1 green pixel
                unsigned char color[] = {0, 255, 0, 255};
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                return defaultTexID;
            }
        }
    
        unsigned int textureID;
        glGenTextures(1, &textureID);
    
        int width, height, nrComponents;
        unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;  // This is the important part for alpha channels

            glBindTexture(GL_TEXTURE_2D, textureID);
            
            // Ensure we're using an internal format that supports alpha if we have 4 components
            GLenum internalFormat = (nrComponents == 4) ? GL_RGBA : format;
            
            // Use the correct internal format
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            
            glGenerateMipmap(GL_TEXTURE_2D);
    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
            stbi_image_free(data);
            
            std::cout << "Texture loaded successfully: " << textureID << " (" << width << "x" << height 
                      << ", " << nrComponents << " channels)" << std::endl;
        }
        else
        {
            std::cout << "Texture failed to load at path: " << filename << std::endl;
            std::cout << "STB Error: " << stbi_failure_reason() << std::endl;
            stbi_image_free(data);
        }
    
        return textureID;
    }
};

#endif
