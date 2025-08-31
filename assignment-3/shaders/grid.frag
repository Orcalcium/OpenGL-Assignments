#version 450 core

in vec3 worldPos;
in vec3 viewPos;

out vec4 FragColor;

float GridFactor(vec3 worldPos) {
    const vec2 HALF_LINE_WIDTH = vec2(0.05f, 0.05f);
    // make it repeat
    vec2 fr = fract(worldPos.xz);
    // [0.0, 1.0) -> [-1.0, 1.0)
    fr = 2.0f * fr - 1.0f;
    // fwidth = abs(dFdx) + abs(dFdy)
    vec2 uvDeriv = fwidth(worldPos.xz);
    const vec2 lineAA = uvDeriv * 1.5f;
    // (approximate) constant pixel width
    vec2 finalHalfLineWidth = HALF_LINE_WIDTH * lineAA;
    // prevent from too thickness
    finalHalfLineWidth = max(HALF_LINE_WIDTH, finalHalfLineWidth);
    
    const vec2 SX = vec2(0.0f, 0.0f);
    const vec2 s = smoothstep(SX - finalHalfLineWidth - lineAA, SX - finalHalfLineWidth, fr);
    const vec2 m = smoothstep(SX + finalHalfLineWidth, SX + finalHalfLineWidth + lineAA, fr);
    const vec2 res = s - m;
    // fade out the line based on the thickness
    const vec2 fadeOutRes = res * clamp(HALF_LINE_WIDTH / finalHalfLineWidth, vec2(0.0f), vec2(1.0f));
    // for the area that is usually has Moire patterns, fade out it
    const vec2 deriv2 = clamp(uvDeriv, 0.0f, 1.0f);
    const vec2 finalRes = mix(fadeOutRes, HALF_LINE_WIDTH, deriv2);
    
    return mix(finalRes.x, 1.0, finalRes.y);
}

vec4 WithFog(vec4 color) {
    const vec4 FOG_COLOR = vec4(0.0, 0.0, 0.0, 1.0);
    const float MAX_DIST = 200.0;
    const float MIN_DIST = 100.0;
    
    float dis = length(viewPos);
    float fogFactor = (MAX_DIST - dis) / (MAX_DIST - MIN_DIST);
    fogFactor = clamp(fogFactor, 0.0f, 1.0f);
    
    return mix(FOG_COLOR, color, fogFactor);
}

void main() {
    const vec4 groundColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const vec4 lineColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
    
    float gridFactor = GridFactor(worldPos);
    vec4 color = mix(groundColor, lineColor, gridFactor);
    
    FragColor = WithFog(color);
}