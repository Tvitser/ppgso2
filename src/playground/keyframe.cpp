#include <glm/glm.hpp>

#include "keyframe.h"
#include "object.h"

Keyframe::Keyframe(glm::mat4 matrix, float duration) {
    this->matrix = matrix;
    this->duration = duration;
}

Keyframe::Keyframe(float duration, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, bool easeIn, bool easeOut) {
    this->duration = duration;
    this->position = position;
    this->rotation = rotation;
    this->scale = scale;
    this->matrix = Object::getModelMatrix(position, rotation, scale);
    this->easeIn = easeIn;
    this->easeOut = easeOut;
}

Keyframe::Keyframe(float duration, glm::vec3 position, glm::vec3 rotation, bool easeIn, bool easeOut) {
    this->duration = duration;
    this->position = position;
    this->rotation = rotation;
    this->scale = {1, 1, 1};
    this->matrix = Object::getModelMatrix(position, rotation, scale);
    this->easeIn = easeIn;
    this->easeOut = easeOut;
}