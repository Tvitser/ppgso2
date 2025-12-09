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
uniform bool useInstancing;
uniform float instanceRadius;
uniform int instanceSeed;
const float UINT_MAX_FLOAT = 4294967295.0;
const float TWO_PI = 6.28318530718;
const uint LCG_A = 1664525u;
const uint LCG_C = 1013904223u;

// Simple integer hash for reproducible pseudo-random values
uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float rand01(uint x) {
    return float(hash(x)) / UINT_MAX_FLOAT;
}

vec2 randomOffset(uint id) {
    uint base = id ^ uint(instanceSeed);
    float r = sqrt(rand01(base)) * instanceRadius;
    float theta = rand01(base * LCG_A + LCG_C) * TWO_PI;
    return vec2(cos(theta), sin(theta)) * r;
}

void main()
{
    mat4 instanceModel = model;
    if (useInstancing) {
        vec2 offset = randomOffset(uint(gl_InstanceID));
        mat4 translateMat = mat4(1.0);
        translateMat[3] = vec4(offset.x, 0.0, offset.y, 1.0);
        instanceModel = translateMat * model;
    }

    vec4 worldPos = instanceModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
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
