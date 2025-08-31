#include "foliage_renderer.h"
#include "obj_loader.h"
#include "spatial_sample_loader.h"
#include "shader_code_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <chrono>

#include "../include/stb/stb_image.h"

FoliageRenderer::FoliageRenderer() 
    : m_instanceSSBO(0), m_drawCommandSSBO(0), m_indirectBuffer(0), 
      m_visibleInstanceSSBO(0), m_counterSSBO(0),
      m_textureArray(0), m_renderShader(0), m_frustumCullingShader(0),
      m_instanceUpdateShader(0), m_textureCount(0),
      m_frustumVAO(0), m_frustumVBO(0), m_frustumShader(0), m_frustumInitialized(false) {
    srand(time(nullptr));
}

FoliageRenderer::~FoliageRenderer() {
    if(m_instanceSSBO) glDeleteBuffers(1, &m_instanceSSBO);
    if(m_drawCommandSSBO) glDeleteBuffers(1, &m_drawCommandSSBO);
    if(m_indirectBuffer) glDeleteBuffers(1, &m_indirectBuffer);
    if(m_visibleInstanceSSBO) glDeleteBuffers(1, &m_visibleInstanceSSBO);
    if(m_counterSSBO) glDeleteBuffers(1, &m_counterSSBO);
    if(m_textureArray) glDeleteTextures(1, &m_textureArray);
    if(m_renderShader) glDeleteProgram(m_renderShader);
    if(m_frustumCullingShader) glDeleteProgram(m_frustumCullingShader);
    if(m_instanceUpdateShader) glDeleteProgram(m_instanceUpdateShader);
    if(m_frustumVAO) glDeleteVertexArrays(1, &m_frustumVAO);
    if(m_frustumVBO) glDeleteBuffers(1, &m_frustumVBO);
    if(m_frustumShader) glDeleteProgram(m_frustumShader);
    
    for(auto& mesh : m_meshes) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }
}

bool FoliageRenderer::initialize() {
    const std::string foliageVertexShader = ShaderCodeLoader::loadShaderCode("shaders/foliage.vert");
    const std::string fragmentShader = ShaderCodeLoader::loadShaderCode("shaders/foliage.frag");
    m_renderShader = createShaderProgram(foliageVertexShader, fragmentShader);
    if(!m_renderShader) {
        std::cerr << "Failed to create render shader" << std::endl;
        return false;
    }

    const std::string computeSrc = ShaderCodeLoader::loadShaderCode("shaders/foliage_cull.comp");
    m_frustumCullingShader = createComputeShader(computeSrc);
    if(!m_frustumCullingShader){
        std::cerr << "GPU culling compute shader failed" << std::endl;
        m_gpuCullingEnabled = false;
    }

    const std::string buildCmdSrc = ShaderCodeLoader::loadShaderCode("shaders/build_cmd.comp");
    m_instanceUpdateShader = createComputeShader(buildCmdSrc);
    
    MeshData grassMesh, bush01Mesh, bush05Mesh;
    
    if(!loadMesh("assets/models/foliages/grassB.obj", grassMesh) ||
       !loadMesh("assets/models/foliages/bush01_lod2.obj", bush01Mesh) ||
       !loadMesh("assets/models/foliages/bush05_lod2.obj", bush05Mesh)) {
        std::cerr << "Failed to load foliage meshes" << std::endl;
        return false;
    }
    
    m_meshes = {grassMesh, bush01Mesh, bush05Mesh};
    
    // Setup texture array
    if(!loadTextures()) {
        std::cerr << "Failed to load textures" << std::endl;
        return false;
    }
    
    return true;
}

void FoliageRenderer::createQuadMesh(MeshData& meshData) {
    std::vector<float> vertices = {
        // positions         // normals           // texture coords
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         0.5f, 1.0f,  0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, 1.0f,  0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };
    
    meshData.indices = {0, 1, 2, 2, 3, 0};
    meshData.indexCount = meshData.indices.size();
    meshData.baseVertex = 0;
    
    glGenVertexArrays(1, &meshData.VAO);
    glGenBuffers(1, &meshData.VBO);
    glGenBuffers(1, &meshData.EBO);
    
    glBindVertexArray(meshData.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(GLuint), meshData.indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void FoliageRenderer::loadPoissonSamples(const std::string& filename) {
    std::vector<SpatialSamplePoint> samples;
    if (!SpatialSampleLoader::loadSS2File(filename, samples)) {
        std::cerr << "Failed to load spatial samples from " << filename << std::endl;
        return;
    }
    
    m_instances.clear();
    m_instances.reserve(samples.size());
    m_activeInstanceIndices.clear();
    m_activeInstancePositions.clear();
    m_activeInstancePositions.reserve(samples.size());
    
    // Convert spatial samples to instances
    for (const auto& sample : samples) {
        InstanceData instance;
        instance.position = glm::vec3(sample.position.x, 0.0f, sample.position.z);
        instance.rotation = sample.rotation.y;
        // Randomly assign mesh types
        float rand_val = static_cast<float>(rand()) / RAND_MAX;
        if (rand_val < 0.98f) {
            instance.meshType = 0;
            instance.textureIndex = 0;
        } else if (rand_val < 0.99f) {
            instance.meshType = 1;
            instance.textureIndex = 1;
        } else {
            instance.meshType = 2;
            instance.textureIndex = 2;
        }
        
        instance.isActive = true;
        instance.isVisible = true;
        
        instance.modelMatrix = glm::mat4(1.0f);
        instance.modelMatrix = glm::translate(instance.modelMatrix, instance.position);
        instance.modelMatrix = glm::rotate(instance.modelMatrix, instance.rotation, glm::vec3(0, 1, 0));

        uint32_t newIndex = (uint32_t)m_instances.size();
        m_instances.push_back(instance);
        m_activeInstancePositions.push_back((int)m_activeInstanceIndices.size());
        m_activeInstanceIndices.push_back(newIndex);
    }
    
    size_t grassCount=0,bush01Count=0,bush05Count=0; 
    for(auto &inst : m_instances){ if(inst.meshType==0) grassCount++; else if(inst.meshType==1) bush01Count++; else if(inst.meshType==2) bush05Count++; }
    std::cout << "Foliage distribution: grass=" << grassCount << " bush01=" << bush01Count << " bush05=" << bush05Count << std::endl;
}

void FoliageRenderer::setupInstanceBuffers(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    if(!m_gpuCullingEnabled){
        m_gpuInstances.clear();
        m_gpuInstances.reserve(m_instances.size());
        GLuint runningBase = 0;
        for(int meshType=0; meshType<3; ++meshType){
            auto &mesh = m_meshes[meshType];
            mesh.baseInstance = runningBase;
            mesh.instanceCount = 0;
            for(const auto &inst : m_instances){
                if(inst.meshType==meshType && inst.isActive && inst.isVisible){
                    GPUInstancePacked packed; 
                    packed.model = inst.modelMatrix; 
                    packed.info = glm::vec4(static_cast<float>(inst.textureIndex), static_cast<float>(inst.meshType),0,0);
                    m_gpuInstances.push_back(packed); 
                    mesh.instanceCount++; 
                    runningBase++;
                }
            }
        }
        updateInstanceSSBO();
    }
}

bool FoliageRenderer::loadTextures() {
    const std::vector<std::string> texturePaths = {
        "assets/textures/grassB_albedo.png",
        "assets/textures/bush01.png", 
        "assets/textures/bush05.png"
    };

    // Load first texture to get sizes
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texturePaths[0].c_str(), &width, &height, &channels, 4); // Force 4 channels
    
    if(!data) {
        std::cerr << "Failed to load texture: " << texturePaths[0] << std::endl;
        return false;
    }
    
    // Determine number of mip levels
    int mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(width, height))));
    
    glGenTextures(1, &m_textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 
                   mipLevels,
                   GL_RGBA8, width, height, static_cast<GLsizei>(texturePaths.size()));
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    // Upload remaining textures
    for(size_t i = 1; i < texturePaths.size(); i++) {
        int w, h, c;
        data = stbi_load(texturePaths[i].c_str(), &w, &h, &c, 4); // Force 4 channels
        if(!data) {
            std::cerr << "Failed to load texture: " << texturePaths[i] << std::endl;
            continue;
        }
        
        std::cout << "Loaded texture: " << texturePaths[i] << " (" << w << "x" << h << ", " << c << " channels)" << std::endl;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        stbi_image_free(data);
    }
    
    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
    
    m_textureCount = texturePaths.size();
    return true;
}

void FoliageRenderer::updateInstanceSSBO() {
    if(m_instanceSSBO == 0) {
        glGenBuffers(1, &m_instanceSSBO);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
    GLsizeiptr requiredSize = static_cast<GLsizeiptr>(m_gpuInstances.size() * sizeof(GPUInstancePacked));
    static GLsizeiptr allocated = 0;
    if(requiredSize > allocated) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, requiredSize, m_gpuInstances.data(), GL_DYNAMIC_DRAW);
        allocated = requiredSize;
    } else {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, requiredSize, m_gpuInstances.data());
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void FoliageRenderer::initializeFrustumVisualization() {
    if(m_frustumInitialized) return;
    
    const std::string frustumVertexShader = ShaderCodeLoader::loadShaderCode("shaders/frustum.vert");
    const std::string frustumFragmentShader = ShaderCodeLoader::loadShaderCode("shaders/frustum.frag");

    m_frustumShader = createShaderProgram(frustumVertexShader, frustumFragmentShader);
    if(!m_frustumShader) {
        std::cerr << "Failed to create frustum visualization shader" << std::endl;
        return;
    }
    
    glGenVertexArrays(1, &m_frustumVAO);
    glGenBuffers(1, &m_frustumVBO);
    
    glBindVertexArray(m_frustumVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_frustumVBO);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    m_frustumInitialized = true;
}

void FoliageRenderer::renderFrustumFrame(const glm::mat4& view, const glm::mat4& projection,
                                         const glm::mat4& playerView, const glm::mat4& playerProjection) {
    if(!m_frustumInitialized) {
        initializeFrustumVisualization();
    }
    
    if(!m_frustumShader) return;
    
    // Calculate frustum corners using player camera's viewProjection matrix
    glm::mat4 playerViewProjection = playerProjection * playerView;
    std::vector<glm::vec3> frustumCorners = calculateFrustumCorners(playerViewProjection);

    std::vector<glm::vec3> frustumLines;
    
    // Near plane
    frustumLines.push_back(frustumCorners[0]); frustumLines.push_back(frustumCorners[1]);
    frustumLines.push_back(frustumCorners[1]); frustumLines.push_back(frustumCorners[2]);
    frustumLines.push_back(frustumCorners[2]); frustumLines.push_back(frustumCorners[3]);
    frustumLines.push_back(frustumCorners[3]); frustumLines.push_back(frustumCorners[0]);
    
    // Far plane
    frustumLines.push_back(frustumCorners[4]); frustumLines.push_back(frustumCorners[5]);
    frustumLines.push_back(frustumCorners[5]); frustumLines.push_back(frustumCorners[6]);
    frustumLines.push_back(frustumCorners[6]); frustumLines.push_back(frustumCorners[7]);
    frustumLines.push_back(frustumCorners[7]); frustumLines.push_back(frustumCorners[4]);

    // Connecting lines
    frustumLines.push_back(frustumCorners[0]); frustumLines.push_back(frustumCorners[4]);
    frustumLines.push_back(frustumCorners[1]); frustumLines.push_back(frustumCorners[5]);
    frustumLines.push_back(frustumCorners[2]); frustumLines.push_back(frustumCorners[6]);
    frustumLines.push_back(frustumCorners[3]); frustumLines.push_back(frustumCorners[7]);
    
    glBindVertexArray(m_frustumVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_frustumVBO);
    glBufferData(GL_ARRAY_BUFFER, frustumLines.size() * sizeof(glm::vec3), frustumLines.data(), GL_DYNAMIC_DRAW);
    
    glUseProgram(m_frustumShader);
    glUniformMatrix4fv(glGetUniformLocation(m_frustumShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_frustumShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Enable wireframe mode and disable depth testing for visualization
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);
    
    glDrawArrays(GL_LINES, 0, frustumLines.size());
    
    // Restore render state
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    
    glBindVertexArray(0);
}

std::vector<glm::vec3> FoliageRenderer::calculateFrustumCorners(const glm::mat4& viewProjectionMatrix) {
    // Calculate the 8 corners of the frustum in world space
    std::vector<glm::vec3> corners(8);
    
    // Inverse of view-projection matrix
    glm::mat4 invViewProj = glm::inverse(viewProjectionMatrix);
    
    // NDC coordinates of frustum corners
    std::vector<glm::vec4> ndcCorners = {
        // Near plane (z = -1 in NDC)
        glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
        glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f),
        glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f),
        glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f),
        
        // Far plane (z = 1 in NDC)
        glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
        glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
        glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f),
        glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f)
    };
    
    for(size_t i = 0; i < 8; ++i) {
        glm::vec4 worldCorner = invViewProj * ndcCorners[i];
        corners[i] = glm::vec3(worldCorner) / worldCorner.w; // Perspective divide
    }
    
    return corners;
}

GLuint FoliageRenderer::createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vSource = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &vSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fSource = fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &fSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        return 0;
    }
    
    // Create program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

GLuint FoliageRenderer::createComputeShader(const std::string& source) {
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    glShaderSource(computeShader, 1, &src, nullptr);
    glCompileShader(computeShader);
    
    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
        std::cerr << "Compute shader compilation failed: " << infoLog << std::endl;
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Compute shader program linking failed: " << infoLog << std::endl;
        return 0;
    }
    
    glDeleteShader(computeShader);
    return program;
}

std::string FoliageRenderer::loadShaderFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if(!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void FoliageRenderer::performFrustumCulling(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    m_lastCameraPos = cameraPos;
    m_lastPlayerView = view;
    m_lastPlayerProjection = projection;
    
    // Compute view-projection matrix
    glm::mat4 viewProjection = projection * view;
    m_lastViewProjection = viewProjection;
    m_lastCameraPos = cameraPos;

    // Extract frustum planes
    std::vector<glm::vec4> frustumPlanes = extractFrustumPlanes(viewProjection);
    const float boundingRadius = 1.5f; // uniform sphere radius (could vary per mesh)
    
    uint32_t visibleCount = 0;
    uint32_t totalActiveCount = 0;
    
    for(auto& instance : m_instances) {
        if(!instance.isActive) {
            instance.isVisible = false;
            continue;
        }
        
        totalActiveCount++;
        
        bool insideFrustum = true;
        // Plane tests
        for(const auto& plane : frustumPlanes) {
            float distance = plane.x * instance.position.x +
                             plane.y * instance.position.y +
                             plane.z * instance.position.z +
                             plane.w;
            if(distance < -boundingRadius) { // sphere entirely outside
                insideFrustum = false;
                break;
            }
        }
        instance.isVisible = insideFrustum;
        if(instance.isVisible) {
            visibleCount++;
        }
    }
}

std::vector<glm::vec4> FoliageRenderer::extractFrustumPlanes(const glm::mat4& viewProjectionMatrix) {
    std::vector<glm::vec4> planes(6);
    
    const float* m = glm::value_ptr(viewProjectionMatrix);

    planes[0] = glm::vec4(m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);
    planes[1] = glm::vec4(m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);
    planes[2] = glm::vec4(m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);
    planes[3] = glm::vec4(m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);
    planes[4] = glm::vec4(m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);
    planes[5] = glm::vec4(m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);
    
    // Normalize the planes
    for(auto& plane : planes) {
        float length = glm::length(glm::vec3(plane));
        if(length > 0.0f) {
            plane /= length;
        }
    }
    
    return planes;
}

void FoliageRenderer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos,
                             const glm::mat4& playerView, const glm::mat4& playerProjection, const glm::vec3& playerPos) {
    if(m_instances.empty()) {
        return;
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = t0;
    auto t2 = t0;
    if(m_gpuCullingEnabled){
        if(m_sourceInstanceSSBO==0){
            rebuildSourceInstanceBuffer();
        }
        dispatchComputeCulling(playerView, playerProjection, playerPos);
        t1 = std::chrono::high_resolution_clock::now();
        t2 = t1;
    } else {
        performFrustumCulling(playerView, playerProjection, playerPos);
        t1 = std::chrono::high_resolution_clock::now();
        setupInstanceBuffers(playerView, playerProjection, playerPos);
        t2 = std::chrono::high_resolution_clock::now();
    }
    
    glUseProgram(m_renderShader);
    
    glUniformMatrix4fv(glGetUniformLocation(m_renderShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_renderShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(m_renderShader, "viewPos"), 1, glm::value_ptr(cameraPos));
    glUniform3f(glGetUniformLocation(m_renderShader, "lightDir"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(m_renderShader, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(m_renderShader, "mipBias"), 0.0f); // Adjust if needed
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray);
    glUniform1i(glGetUniformLocation(m_renderShader, "textureArray"), 0);
    
    buildCombinedBuffers();
    auto t3 = std::chrono::high_resolution_clock::now();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_instanceSSBO);
    glBindVertexArray(m_combinedVAO);
    updateIndirectBuffer();
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
    // Single multi-draw call (DrawElementsIndirectCommand array already laid out)
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)m_meshes.size(), 0);
    auto t4 = std::chrono::high_resolution_clock::now();
    using msd = std::chrono::duration<double, std::milli>;
    m_profileData.cpuCullMs = std::chrono::duration_cast<msd>(t1 - t0).count();
    m_profileData.cpuSetupMs = std::chrono::duration_cast<msd>(t2 - t1).count();
    m_profileData.cpuDrawMs = std::chrono::duration_cast<msd>(t4 - t3).count();
    
    renderFrustumFrame(view, projection, playerView, playerProjection);
}

void FoliageRenderer::checkCollisions(const glm::vec3& playerPos, float playerRadius) {
    auto c0 = std::chrono::high_resolution_clock::now();
    bool needsUpdate = false;
    m_profileData.collisionTests = 0;
    m_profileData.collisionHits = 0;
    for(size_t i = 0; i < m_activeInstanceIndices.size(); ) {
        uint32_t instIdx = m_activeInstanceIndices[i];
        InstanceData &instance = m_instances[instIdx];
        m_profileData.collisionTests++;
        float foliageRadius = (instance.meshType == 1 || instance.meshType == 2) ? 0.75f : 0.45f;
        float testRadius = playerRadius + foliageRadius;
        float dx = playerPos.x - instance.position.x;
        float dy = playerPos.y - instance.position.y;
        float dz = playerPos.z - instance.position.z;
        float distSq = dx*dx + dy*dy + dz*dz;
        if(distSq < testRadius * testRadius){
            instance.isActive = false;
            needsUpdate = true;
            m_profileData.collisionHits++;
            // swap-remove this index
            uint32_t backIdx = m_activeInstanceIndices.back();
            m_activeInstanceIndices[i] = backIdx;
            m_activeInstancePositions[backIdx] = (int)i;
            m_activeInstanceIndices.pop_back();
            m_activeInstancePositions[instIdx] = -1;
            continue; // do not advance i, new element at i to test
        }
        ++i; // advance only when not removed
    }
    auto c1 = std::chrono::high_resolution_clock::now();
    using msd = std::chrono::duration<double, std::milli>;
    m_profileData.cpuCollisionLoopMs = std::chrono::duration_cast<msd>(c1 - c0).count();
    double rebuildMs = 0.0;
    if(needsUpdate) {
        auto r0 = std::chrono::high_resolution_clock::now();
        if(m_gpuCullingEnabled){
            rebuildSourceInstanceBuffer();
        } else {
            setupInstanceBuffers(m_lastPlayerView, m_lastPlayerProjection, m_lastCameraPos);
        }
        auto r1 = std::chrono::high_resolution_clock::now();
        rebuildMs = std::chrono::duration_cast<msd>(r1 - r0).count();
    }
    m_profileData.cpuCollisionRebuildMs = rebuildMs;
}

bool FoliageRenderer::loadMesh(const std::string& objPath, MeshData& meshData) {
    Mesh mesh;
    if (!SimpleOBJLoader::loadOBJ(objPath, mesh)) {
        std::cerr << "Failed to load OBJ file: " << objPath << std::endl;
        return false;
    }

    std::vector<float> vertices;
    
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const auto& vertex = mesh.vertices[i];
        
        // Position
        vertices.push_back(vertex.position.x);
        vertices.push_back(vertex.position.y);
        vertices.push_back(vertex.position.z);
        
        // Normal
        vertices.push_back(vertex.normal.x);
        vertices.push_back(vertex.normal.y);
        vertices.push_back(vertex.normal.z);
        
        // Texture coordinates
        vertices.push_back(vertex.texCoords.x);
        vertices.push_back(vertex.texCoords.y);
    }
    
    meshData.indices = mesh.indices;
    meshData.vertexData = vertices; // store for combined buffer build
    meshData.indexCount = mesh.indices.size();
    meshData.baseVertex = 0;
    meshData.instanceCount = 0; // Initialize to 0, will be set in setupInstanceBuffers

    glGenVertexArrays(1, &meshData.VAO);
    glGenBuffers(1, &meshData.VBO);
    glGenBuffers(1, &meshData.EBO);
    
    glBindVertexArray(meshData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(GLuint), meshData.indices.data(), GL_STATIC_DRAW);
    
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    return true;
}

// Build combined VBO/EBO and setup a single VAO for multi-draw indirect
void FoliageRenderer::buildCombinedBuffers(){
    if(m_combinedBuilt) return;
    std::vector<float> allVertices;
    std::vector<GLuint> allIndices;
    allVertices.reserve(1024);
    allIndices.reserve(1024);
    GLuint vertexOffset = 0;
    for(auto &mesh : m_meshes){
        mesh.baseVertex = vertexOffset / 8;
        mesh.firstIndex = allIndices.size();
        allVertices.insert(allVertices.end(), mesh.vertexData.begin(), mesh.vertexData.end());
        // Remap indices
        for(auto idx : mesh.indices){
            GLuint remapped = idx + mesh.baseVertex;
            allIndices.push_back(remapped);
        }
        vertexOffset += mesh.vertexData.size();
    }
    glGenVertexArrays(1, &m_combinedVAO);
    glGenBuffers(1, &m_combinedVBO);
    glGenBuffers(1, &m_combinedEBO);
    glBindVertexArray(m_combinedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_combinedVBO);
    glBufferData(GL_ARRAY_BUFFER, allVertices.size()*sizeof(float), allVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_combinedEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size()*sizeof(GLuint), allIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    m_combinedBuilt = true;
}

void FoliageRenderer::updateIndirectBuffer(){
    struct IndirectCommand { GLuint count; GLuint instanceCount; GLuint firstIndex; GLuint baseVertex; GLuint baseInstance; };
    std::vector<IndirectCommand> commands;
    commands.reserve(m_meshes.size());

    for(auto &mesh : m_meshes){
        IndirectCommand cmd{};
        cmd.count = mesh.indexCount;
        cmd.instanceCount = mesh.instanceCount;
        cmd.firstIndex = mesh.firstIndex;
        cmd.baseVertex = 0;
        cmd.baseInstance = mesh.baseInstance;
        commands.push_back(cmd);
    }
    
    if(!m_indirectBuffer) glGenBuffers(1, &m_indirectBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, commands.size()*sizeof(IndirectCommand), commands.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void FoliageRenderer::rebuildSourceInstanceBuffer(){
    std::vector<GPUInstancePacked> source;
    source.reserve(m_activeInstanceIndices.size());
    m_meshActiveCounts.assign(m_meshes.size(), 0);
    for(uint32_t instIdx : m_activeInstanceIndices){
        InstanceData &inst = m_instances[instIdx];
        GPUInstancePacked packed{};
        packed.model = inst.modelMatrix;
        packed.info = glm::vec4((float)inst.textureIndex, (float)inst.meshType,0,0);
        source.push_back(packed);
        m_meshActiveCounts[inst.meshType]++;
    }

    if(m_sourceInstanceSSBO==0) glGenBuffers(1,&m_sourceInstanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sourceInstanceSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, source.size()*sizeof(GPUInstancePacked), source.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);

    GLuint totalActive = 0; for(auto c : m_meshActiveCounts) totalActive += c;
    if(m_instanceSSBO==0) glGenBuffers(1,&m_instanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
    static GLsizeiptr allocated=0;
    GLsizeiptr req = (GLsizeiptr)totalActive * sizeof(GPUInstancePacked);

    if(req>allocated){ glBufferData(GL_SHADER_STORAGE_BUFFER, req, nullptr, GL_DYNAMIC_DRAW); allocated=req; }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);

    if(m_counterSSBO==0) glGenBuffers(1,&m_counterSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_counterSSBO);
    std::vector<GLuint> zeros(m_meshes.size(),0);
    glBufferData(GL_SHADER_STORAGE_BUFFER, zeros.size()*sizeof(GLuint), zeros.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);
}

void FoliageRenderer::dispatchComputeCulling(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos){
    if(!m_frustumCullingShader) return;
    auto t0 = std::chrono::high_resolution_clock::now();
    // Reset per-mesh visible counters
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_counterSSBO);
    std::vector<GLuint> zeroCounters(m_meshes.size(), 0);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, zeroCounters.size()*sizeof(GLuint), zeroCounters.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Compute baseOffsets & capacities
    std::vector<GLuint> baseOffsets(m_meshes.size(),0);
    std::vector<GLuint> capacities(m_meshes.size(),0);
    GLuint running = 0;
    for(size_t i=0;i<m_meshes.size();++i){
        baseOffsets[i] = running;
        capacities[i] = m_meshActiveCounts[i];
        running += m_meshActiveCounts[i];
    }

    glUseProgram(m_frustumCullingShader);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_sourceInstanceSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_instanceSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_counterSSBO);
    GLuint total = 0; for(auto c: m_meshActiveCounts) total += c;
    glUniformMatrix4fv(glGetUniformLocation(m_frustumCullingShader,"uView"),1,GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_frustumCullingShader,"uProj"),1,GL_FALSE, glm::value_ptr(projection));
    glUniform1ui(glGetUniformLocation(m_frustumCullingShader,"uTotalInstances"), total);
    glUniform1ui(glGetUniformLocation(m_frustumCullingShader,"uMeshCount"), (GLuint)m_meshes.size());
    glUniform3fv(glGetUniformLocation(m_frustumCullingShader,"uPlayerPos"),1, glm::value_ptr(cameraPos));
    glUniform1f(glGetUniformLocation(m_frustumCullingShader,"uCullDistance"), 100.0f);
    // Upload baseOffsets & capacities arrays
    GLint baseLoc = glGetUniformLocation(m_frustumCullingShader, "uBaseOffsets");
    if(baseLoc >= 0) glUniform1uiv(baseLoc, (GLsizei)m_meshes.size(), baseOffsets.data());
    GLint capLoc = glGetUniformLocation(m_frustumCullingShader, "uCapacities");
    if(capLoc >= 0) glUniform1uiv(capLoc, (GLsizei)m_meshes.size(), capacities.data());
    GLuint groups = (total + 127)/128;
    glDispatchCompute(groups,1,1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    auto t1 = std::chrono::high_resolution_clock::now();
    // Read back visible counts
    std::vector<GLuint> counts(m_meshes.size(),0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_counterSSBO);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,0, counts.size()*sizeof(GLuint), counts.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);
    auto t2 = std::chrono::high_resolution_clock::now();
    // Assign baseInstance (from prefix) & instanceCount (visible)
    running = 0;
    for(size_t i=0;i<m_meshes.size();++i){
        m_meshes[i].baseInstance = baseOffsets[i];
        m_meshes[i].instanceCount = counts[i];
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    using msd = std::chrono::duration<double, std::milli>;
    m_profileData.cpuDispatchMs = std::chrono::duration_cast<msd>(t1 - t0).count();
    m_profileData.cpuReadbackMs = std::chrono::duration_cast<msd>(t2 - t1).count();
    m_profileData.cpuReorderMs = 0.0; // eliminated CPU reorder
}
