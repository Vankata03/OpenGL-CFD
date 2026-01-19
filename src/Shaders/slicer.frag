#version 330 core
out vec4 FragColor;

in float v_WorldZ;

uniform float sliceZ;
uniform float thickness;

void main()
{
    if (abs(v_WorldZ - sliceZ) > thickness * 0.5)
        discard;

    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
