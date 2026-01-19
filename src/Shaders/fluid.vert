#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 viewProjection;
uniform vec2 gridSize;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    vec2 worldPos = aPos * gridSize;
    gl_Position = viewProjection * vec4(worldPos, 0.0, 1.0);
}
