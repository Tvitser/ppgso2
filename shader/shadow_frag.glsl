#version 330 core
in vec3 FragPos;
uniform vec3 lightPos;
uniform float far_plane;
uniform bool isPointLight;

void main() {
    if (isPointLight) {
        float lightDistance = length(FragPos - lightPos);
        // map to [0,1] range for depth comparison in cube map
        lightDistance = lightDistance / far_plane;
        gl_FragDepth = lightDistance;
    }
}