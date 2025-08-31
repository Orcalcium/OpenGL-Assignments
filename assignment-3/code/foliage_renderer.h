#pragma once

#include "../include/glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

struct InstanceData {
    glm::mat4 modelMatrix;
    glm::vec3 position;
    float rotation;
    int meshType; // 0=grass, 1=bush01, 2=bush05
    int textureIndex;
    bool isActive;
    bool isVisible;
};

// Draw command structure for glMultiDrawElementsIndirect
struct DrawCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t baseVertex;
    uint32_t baseInstance;
};

// GPU instance data structure
struct GPUInstanceData {
    glm::mat4 modelMatrix;
    float textureIndex;
    uint32_t meshType;
    uint32_t isActive;
    uint32_t isVisible;
    glm::vec3 position;
    float rotation;
    // Padding to obey std430 standard
    float padding[2];
};

class FoliageRenderer {
public:
    FoliageRenderer();
    ~FoliageRenderer();

    bool initialize();
    void loadPoissonSamples(const std::string& filename);
    // Rendering functions
    void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos,
                const glm::mat4& playerView, const glm::mat4& playerProjection, const glm::vec3& playerPos);
    // Collision detection and interaction
    void checkCollisions(const glm::vec3& playerPos, float playerRadius = 0.5f);
    // Frustum
    void renderFrustumFrame(const glm::mat4& view, const glm::mat4& projection,
                           const glm::mat4& playerView, const glm::mat4& playerProjection);
    // Compute shader functions
    void performFrustumCulling(const glm::mat4& viewProjection);
    void updateInstances();

    struct ProfileData {
        double cpuCullMs = 0.0;
        double cpuSetupMs = 0.0;
        double cpuDrawMs = 0.0;

        double cpuBuildSourceMs = 0.0;
        double cpuDispatchMs = 0.0;
        double cpuReadbackMs = 0.0;
        double cpuReorderMs = 0.0;

        double cpuCollisionLoopMs = 0.0;
        double cpuCollisionRebuildMs = 0.0;
        GLuint collisionTests = 0;
        GLuint collisionHits = 0;
    };
    const ProfileData& getProfileData() const { return m_profileData; }
    void resetProfileData() { m_profileData = {}; }

private:
    // Mesh data
    struct MeshData {
        GLuint VAO, VBO, EBO;
        std::vector<GLuint> indices;
        std::vector<float> vertexData; // stored for building combined buffer
        GLuint indexCount;
        GLuint baseVertex;
        GLuint instanceCount; // Number of instances for this mesh type
        GLuint baseInstance;  // Starting offset in SSBO for this mesh's packed instances
        GLuint firstIndex; // starting index in combined index buffer
    };
    
    std::vector<MeshData> m_meshes;
    std::vector<InstanceData> m_instances;
    // active instance indices for rendering
    std::vector<GLuint> m_activeInstanceIndices;
    // position inside m_activeInstanceIndices
    std::vector<int> m_activeInstancePositions;
    std::vector<DrawCommand> m_drawCommands;
    
    // OpenGL objects
    GLuint m_instanceSSBO;
    GLuint m_drawCommandSSBO;
    GLuint m_indirectBuffer;
    GLuint m_visibleInstanceSSBO;
    GLuint m_counterSSBO;
    GLuint m_textureArray;
    
    // Shaders
    GLuint m_renderShader;
    GLuint m_frustumCullingShader;
    GLuint m_instanceUpdateShader;
    
    // Texture management
    static const int MAX_TEXTURES = 4;
    int m_textureCount;

    bool loadMesh(const std::string& objPath, MeshData& meshData);
    bool loadTextures();
    GLuint createComputeShader(const std::string& source);
    GLuint createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
    void createQuadMesh(MeshData& meshData);
    std::string loadShaderFromFile(const std::string& filename);
    
    // CPU-based frustum culling
    void setupInstanceBuffers(); 
    void setupInstanceBuffers(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    void performFrustumCulling();
    void performFrustumCulling(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    std::vector<glm::vec4> extractFrustumPlanes(const glm::mat4& viewProjectionMatrix);
    void updateInstanceSSBO();
    
    // Frustum visualization functions
    void initializeFrustumVisualization();
    std::vector<glm::vec3> calculateFrustumCorners(const glm::mat4& viewProjectionMatrix);
    
    glm::vec3 m_lastCameraPos;
    glm::mat4 m_lastViewProjection;
    glm::mat4 m_lastPlayerView;
    glm::mat4 m_lastPlayerProjection;
    
    // Frustum visualization
    GLuint m_frustumVAO, m_frustumVBO;
    GLuint m_frustumShader;
    bool m_frustumInitialized;
    ProfileData m_profileData;

    struct GPUInstancePacked {
        glm::mat4 model;
        glm::vec4 info;
    };
    std::vector<GPUInstancePacked> m_gpuInstances;

    // buffers for multi-draw indirect
    GLuint m_combinedVAO = 0;
    GLuint m_combinedVBO = 0;
    GLuint m_combinedEBO = 0;
    bool m_combinedBuilt = false;
    void buildCombinedBuffers();
    void updateIndirectBuffer();
    
    // GPU culling
    bool m_gpuCullingEnabled = true;
    GLuint m_sourceInstanceSSBO = 0; // holds all active instances grouped by mesh
    std::vector<GLuint> m_meshActiveCounts; // total active per mesh (capacity for grouping)
    void rebuildSourceInstanceBuffer();
    void dispatchComputeCulling(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    void rebuildActiveInstanceIndices();
};
