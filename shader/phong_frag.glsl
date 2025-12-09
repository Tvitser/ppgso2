#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace[4];

out vec4 FragColor;

// Используем то же имя, что и в C++: shader->setUniform("Texture", *texture)
uniform sampler2D Texture;
uniform sampler2D shadowMap0;
uniform sampler2D shadowMap1;
uniform sampler2D shadowMap2;
uniform sampler2D shadowMap3;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

struct Light {
    int type; // 0 - directional, 1 - point, 2 - spot/reflector
    vec3 position;
    vec3 direction;
    vec3 color;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float maxDist;
    float cutOff;
    float outerCutOff;
};

const int LIGHT_DIRECTIONAL = 0;
const int LIGHT_POINT       = 1;
const int LIGHT_SPOT        = 2;
const int MAX_LIGHTS        = 10;
const int MAX_SHADOW_MAPS   = 4;
const float SPOT_EPSILON    = 0.0001;
const float MIN_ATTENUATION = 0.0001;
// Minimal facing threshold to allow specular highlights
const float SPECULAR_EPSILON = 0.001;
const float SHADOW_BIAS_MIN = 0.005;
const float SHADOW_BIAS_SLOPE = 0.05;
const int SHADOW_KERNEL_RADIUS = 1;
const int SHADOW_KERNEL_SAMPLES = (2 * SHADOW_KERNEL_RADIUS + 1) * (2 * SHADOW_KERNEL_RADIUS + 1);

uniform int numberOfLights;
uniform int numShadowMaps;
uniform int shadowCasterIndices[4]; // Maps shadow map index to light index
uniform Light lights[MAX_LIGHTS];

uniform vec3 viewPos;
uniform float Transparency;

// ============ ФУНКЦИЯ ТЕНИ ============
float calculateShadowFromMap(sampler2D sMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(SHADOW_BIAS_MIN, SHADOW_BIAS_SLOPE * (1.0 - dot(normal, lightDir)));
    vec2 texelSize = 1.0 / vec2(textureSize(sMap, 0));

    float shadow = 0.0;
    for (int x = -SHADOW_KERNEL_RADIUS; x <= SHADOW_KERNEL_RADIUS; ++x) {
        for (int y = -SHADOW_KERNEL_RADIUS; y <= SHADOW_KERNEL_RADIUS; ++y) {
            float closestDepth = texture(sMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float(SHADOW_KERNEL_SAMPLES);
    return shadow;
}

// Get shadow value for a specific light by checking all shadow maps
float getShadowForLight(int lightIndex, vec3 normal, vec3 lightDir)
{
    for (int i = 0; i < numShadowMaps && i < MAX_SHADOW_MAPS; ++i) {
        if (shadowCasterIndices[i] == lightIndex) {
            // Found shadow map for this light
            if (i == 0) return calculateShadowFromMap(shadowMap0, FragPosLightSpace[0], normal, lightDir);
            if (i == 1) return calculateShadowFromMap(shadowMap1, FragPosLightSpace[1], normal, lightDir);
            if (i == 2) return calculateShadowFromMap(shadowMap2, FragPosLightSpace[2], normal, lightDir);
            if (i == 3) return calculateShadowFromMap(shadowMap3, FragPosLightSpace[3], normal, lightDir);
        }
    }
    return 0.0; // No shadow map for this light
}

vec3 applyLight(int lightIndex, in Light light, in vec3 norm, in vec3 viewDir, in vec3 texColor)
{
    if (light.type == LIGHT_DIRECTIONAL && light.maxDist > 0.0) {
        float distanceFromCamera = length(viewPos - FragPos);
        if (distanceFromCamera > light.maxDist) {
            return vec3(0.0);
        }
    }

    vec3 lightDir;
    float attenuation = 1.0;
    if (light.type == LIGHT_DIRECTIONAL) {
        lightDir = normalize(-light.direction);
    } else {
        vec3 lightVec = light.position - FragPos;
        lightDir = normalize(lightVec);

        float distanceToLight = length(lightVec);
        float denom = light.constant + light.linear * distanceToLight + light.quadratic * (distanceToLight * distanceToLight);
        attenuation = 1.0 / max(denom, MIN_ATTENUATION);
    }

    float ndotl = dot(norm, lightDir);
    float facing = max(ndotl, 0.0);
    vec3 diffuse = light.diffuse * facing * material.diffuse * texColor;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(norm, halfwayDir), 0.0);
    float spec = pow(specAngle, material.shininess);
    // Avoid specular highlights on back-facing fragments
    spec *= step(SPECULAR_EPSILON, facing);
    vec3 specular = light.specular * spec * material.specular;

    vec3 ambient = light.ambient * material.ambient * texColor;

    if (light.type == LIGHT_SPOT) {
        vec3 spotDir = normalize(light.direction);
        vec3 toFragment = normalize(FragPos - light.position);
        float theta = dot(spotDir, toFragment);
        float epsilon = max(light.cutOff - light.outerCutOff, SPOT_EPSILON);
        float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
        diffuse *= intensity;
        specular *= intensity;
    }

    // Calculate shadow from this light's shadow map (if it has one)
    float shadow = getShadowForLight(lightIndex, norm, lightDir);

    vec3 lightColor = light.color;
    return lightColor * (ambient + (1.0 - shadow) * (diffuse + specular)) * attenuation;
}

// ============ MAIN ============
void main()
{
    vec4 texColor = texture(Texture, TexCoords);
    if (texColor.a < 0.01)
        discard;

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 color = vec3(0.0);
    for (int i = 0; i < numberOfLights; ++i) {
        color += applyLight(i, lights[i], norm, viewDir, texColor.rgb);
    }

    FragColor = vec4(color, texColor.a * Transparency);
}
