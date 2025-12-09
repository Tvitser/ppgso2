#ifndef PPGSO_LIGHT_H
#define PPGSO_LIGHT_H

#include <glm/glm.hpp>
#include "object.h"
#include "glm/gtc/matrix_transform.hpp"

enum class LightType {
    Directional = 0,
    Point = 1,
    Spot = 2
};

class Light {
public:
    glm::vec3 position{0, 0, 0};
    LightType type = LightType::Point;

    bool isSpotlight = false;
    // Local orientation set during construction; used as a fallback when explicit direction is missing.
    glm::vec3 localDirection;
    // Resolved world-space direction for directional/spot lights.
    glm::vec3 direction;
    float cutOff = 30.f;
    float outerCutOff = 35.f;
    float maxDist = -1.f;

    float constant = 1.0f;
    float linear = 0.0f;
    float quadratic = 0.0f;

    glm::vec3 color{1, 1, 1};

    Light() = default;

    Light(glm::vec3 color, float constant, float linear, float quadratic, float maxDist);

    Light(glm::vec3 color, glm::vec3 direction, float cutOff, float outerCutOff, float constant, float linear, float quadratic, float maxDist);

    glm::vec3 effectiveDirection() const;
};

#endif //PPGSO_LIGHT_H
