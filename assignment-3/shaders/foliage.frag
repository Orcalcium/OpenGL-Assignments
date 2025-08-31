#version 450 core

in vec3 FragPos;
in vec3 Normal;
in vec3 TexCoord;

uniform sampler2DArray textureArray;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float mipBias; // Negative = sharper, Positive = blurrier

out vec4 FragColor;

void main() {
    // Single biased sample (explicit mipmap usage)
    vec4 texSample = texture(textureArray, TexCoord, mipBias);
    vec3 color = texSample.rgb;
    
    if(length(color) < 0.01) {
        color = vec3(0.2, 0.8, 0.2); // Fallback
    }
    
    if(texSample.a < 0.1) {
        discard;
    }
    
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-vec3(-0.2, -1.0, -0.3));
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 ambient = 0.4 * color;
    vec3 diffuse = 0.6 * diff * color;
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}