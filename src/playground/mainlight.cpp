// C++
#include "mainlight.h"

MainLight::MainLight(glm::vec3 color, float constant, float linear, float quadratic, float maxDist)
    : Light(color, constant, linear, quadratic, maxDist) {
    updateProjection();
}

MainLight::MainLight(glm::vec3 color, glm::vec3 direction, float cutOff, float outerCutOff,
                     float constant, float linear, float quadratic, float maxDist)
    : Light(color, direction, cutOff, outerCutOff, constant, linear, quadratic, maxDist) {
    updateProjection();
}
