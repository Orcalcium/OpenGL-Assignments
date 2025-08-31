#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

struct GPUInstancePacked { mat4 model; vec4 info; }; // info.x = texture index, info.y = meshType
layout(std430, binding = 0) buffer InstanceBuffer { GPUInstancePacked instances[]; };

uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec3 TexCoord;

void main() {
    // gl_BaseInstance provided per draw command in multi-draw indirect
    uint idx = gl_BaseInstance + gl_InstanceID;
    mat4 M = instances[idx].model;
    float tIndex = instances[idx].info.x;
    vec4 worldPos = M * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(M))) * aNormal;
    TexCoord = vec3(aTexCoord, tIndex);
    gl_Position = projection * view * worldPos;
}