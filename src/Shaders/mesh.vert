#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 projection;

out float v_WorldZ;
out vec3 v_Normal;
out vec2 v_TexCoords;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    v_WorldZ = worldPos.z;
    v_Normal = mat3(transpose(inverse(model))) * aNormal;
    v_TexCoords = aTexCoords;
    gl_Position = projection * worldPos;
}
