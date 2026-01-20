#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D velocityXTexture;
uniform sampler2D velocityYTexture;
uniform sampler2D pressureTexture;
uniform sampler2D dyeDensityTexture;
uniform sampler2D solidMaskTexture;

uniform int displayMode; // 0=Dye, 1=Velocity, 2=Pressure

vec3 heatMap(float v) {
    float value = clamp(v, 0.0, 1.0);
    if (value < 0.25) {
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), value * 4.0);
    } else if (value < 0.5) {
        return mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (value - 0.25) * 4.0);
    } else if (value < 0.75) {
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (value - 0.5) * 4.0);
    } else {
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (value - 0.75) * 4.0);
    }
}

void main()
{
    float velocityX = texture(velocityXTexture, TexCoords).r;
    float velocityY = texture(velocityYTexture, TexCoords).r;
    float pressure = texture(pressureTexture, TexCoords).r;
    float dyeDensity = texture(dyeDensityTexture, TexCoords).r;
    float solidMask = texture(solidMaskTexture, TexCoords).r;

    // Check if the pixel is inside a solid object
    if (solidMask > 0.5) {
        FragColor = vec4(0.4, 0.4, 0.4, 1.0);
        return;
    }

    vec3 color = vec3(0.0);

    if (displayMode == 0) {
        vec3 bg = vec3(0.05, 0.05, 0.1);
        color = mix(bg, vec3(1.0), clamp(dyeDensity * 1.5, 0.0, 1.0));
    }
    else if (displayMode == 1) {
        float speed = length(vec2(velocityX, velocityY));
        color = heatMap(speed * 0.5);
    }
    else if (displayMode == 2) {
        // Map [-0.5, 0.5] roughly to [0, 1]
        color = heatMap(pressure * 100.0 + 0.5);
    }

    FragColor = vec4(color, 1.0);
}
