#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aNormal;
layout(location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace[4];

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix[4];
uniform int numShadowMaps;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    Normal = mat3(transpose(inverse(model))) * aNormal;
    // Инвертируем V-координату для корректного отображения текстуры (оставьте или уберите ниже по результатам теста)
    TexCoords = vec2(aTexCoord.x, 1.0 - aTexCoord.y);

    // Compute light space positions for all shadow-casting lights
    for (int i = 0; i < 4; ++i) {
        if (i < numShadowMaps) {
            FragPosLightSpace[i] = lightSpaceMatrix[i] * worldPos;
        } else {
            FragPosLightSpace[i] = vec4(0.0);
        }
    }

    gl_Position = projection * view * worldPos;
}