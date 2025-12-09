#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;


uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

void main() {
    TexCoords = aPos;


    mat4 rotView = mat4(mat3(ViewMatrix));

    vec4 pos = ProjectionMatrix * rotView * vec4(aPos, 1.0);


    gl_Position = pos.xyww;
}
