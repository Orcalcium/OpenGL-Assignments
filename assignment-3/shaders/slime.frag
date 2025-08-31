#version 450 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D slimeTexture;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

out vec4 FragColor;

void main() {
    vec3 color = texture(slimeTexture, TexCoord).rgb;
    
    // Phong shading
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * color;
    FragColor = vec4(result, 1.0);
}