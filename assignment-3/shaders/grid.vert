#version 450 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 viewPos;

void main() {
    worldPos = aPos;
    vec4 viewPosition = view * vec4(aPos, 1.0);
    viewPos = viewPosition.xyz;
    
    gl_Position = projection * viewPosition;
}