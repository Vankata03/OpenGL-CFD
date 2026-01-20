#version 330 core
out vec4 FragColor;

in float v_WorldZ;
in vec3 v_Normal;
in vec2 v_TexCoords;

uniform float sliceZ;
uniform float thickness;
uniform sampler2D meshTexture;

void main()
{
    float dist = abs(v_WorldZ - sliceZ);

    vec3 lightDir = normalize(vec3(0.5, 0.5, 1.0));
    float diff = max(dot(normalize(v_Normal), lightDir), 0.0) * 0.5 + 0.5;

    vec3 baseColor = texture(meshTexture, v_TexCoords).rgb;

    // Fallback if texture is transparent or missing/white
    if (length(baseColor) < 0.1) baseColor = vec3(1.0);

    if (dist < thickness * 0.5) {
        FragColor = vec4(1.0 * diff, 0.0, 0.0, 1.0);
    } else {
        FragColor = vec4(baseColor * diff, 0.3);
    }
}
