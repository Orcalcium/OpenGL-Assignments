#version 330 core
layout (location = 0) in vec3 aPos;      // Position attribute (location 0)
layout (location = 1) in vec3 aNormal;   // Normal attribute (location 1) 
layout (location = 2) in vec2 aTexCoords; // Texture coordinate attribute (location 2)

out vec2 TexCoords;
out vec3 Normal;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

//enum class ViewMode {
//    Scene = 0,
//    Normal,
//    PostProcessing,
//};

void main()
{
    Normal = aNormal; // Pass the normal to the fragment shader
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
