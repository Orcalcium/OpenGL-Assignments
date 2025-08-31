#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal; // Added normal input

//enum class ViewMode {
//    Scene = 0,
//    Normal,
//    PostProcessing,
//};
uniform int viewMode;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoords);
    
    if(texColor.a < 0.1)
        discard;
        
    if(viewMode== 1){
        FragColor = vec4(normalize(Normal) * 0.5 + 0.5, 1.0);
    }
    else{
        FragColor = texColor;
    }
}
