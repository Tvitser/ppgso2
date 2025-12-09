#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 ModelMatrix;
out vec3 FragPos;

void main()
{
    vec4 worldPos = ModelMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = lightSpaceMatrix * worldPos;
}