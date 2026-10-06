#version 330 core
layout(location = 0) in vec2 quadCorner;
layout(location = 1) in vec3 instPos;
layout(location = 2) in vec2 instSize;
layout(location = 3) in vec4 instColor;
layout(location = 4) in vec4 instBrightness;
layout(location = 5) in float instRotation;

uniform mat4 view;
uniform mat4 projection;

out vec4 vColor;
out vec4 vBrightness;
out vec2 vUV;

void main() {
    vec3 worldPos = instPos;
    vec2 size     = instSize;

    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);

    float c = cos(instRotation);
    float s = sin(instRotation);
    vec2 rotated = vec2(
        quadCorner.x * c - quadCorner.y * s,
        quadCorner.x * s + quadCorner.y * c
    );

    vec3 offset = camRight * (rotated.x * size.x) + camUp * (rotated.y * size.y);
    
    gl_Position = projection * view * vec4(worldPos + offset, 1.0);
    vColor = instColor;
    vBrightness = instBrightness;
    vUV = quadCorner + 0.5;
}