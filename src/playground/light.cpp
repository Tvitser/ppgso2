#include <glm/glm.hpp>

#include "light.h"

constexpr float DIRECTIONAL_LIGHT_CUTOFF_THRESHOLD = 0.001f;

Light::Light(glm::vec3 color, float constant, float linear, float quadratic, float maxDist) {
    this->type = LightType::Point;
    this->isSpotlight = false;
    this->localDirection = {0.0f, 0.0f, 0.0f};
    this->direction = {0.0f, 0.0f, 0.0f};
    this->constant = constant;
    this->linear = linear;
    this->quadratic = quadratic;
    this->maxDist = maxDist;
    this->color = color;
}

Light::Light(glm::vec3 color, glm::vec3 direction, float cutOffDeg, float outerCutOffDeg,
             float constant, float linear, float quadratic, float maxDist) {
    const bool isDirectional = cutOffDeg <= DIRECTIONAL_LIGHT_CUTOFF_THRESHOLD && outerCutOffDeg <= DIRECTIONAL_LIGHT_CUTOFF_THRESHOLD;
    this->type = isDirectional ? LightType::Directional : LightType::Spot;
    this->isSpotlight = !isDirectional;

    float dirLength = glm::length(direction);
    auto normalizedDir = dirLength > 0.0f ? direction / dirLength : glm::vec3{0.0f, 0.0f, 0.0f};
    this->localDirection = normalizedDir;
    this->direction = normalizedDir;

    if (isDirectional) {
        this->cutOff = -1.0f;
        this->outerCutOff = -1.0f;
    } else {
        this->cutOff = glm::cos(glm::radians(cutOffDeg));
        this->outerCutOff = glm::cos(glm::radians(outerCutOffDeg));
    }
    this->constant = constant;
    this->linear = linear;
    this->quadratic = quadratic;
    this->maxDist = maxDist;

    this->color = color;
}

glm::vec3 Light::effectiveDirection() const {
    auto chosenDir = glm::length(direction) > 0.0f ? direction : localDirection;
    float chosenDirLength = glm::length(chosenDir);
    if (chosenDirLength > 0.0f) {
        if (chosenDirLength < 0.999f || chosenDirLength > 1.001f) {
            return chosenDir / chosenDirLength;
        }
        return chosenDir;
    }
    return glm::vec3{0.0f, 0.0f, 0.0f};
}
