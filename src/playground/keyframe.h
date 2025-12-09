#ifndef PPGSO_KEYFRAME_H
#define PPGSO_KEYFRAME_H

class Keyframe {
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale{1, 1, 1};
    glm::mat4 matrix;
    float duration;
    bool easeIn = true;
    bool easeOut = true;

    Keyframe(glm::mat4 matrix, float duration);

    Keyframe(float duration, glm::vec3 position, glm::vec3 rotation = {0, 0, 0}, bool easeIn = false, bool easeOut = false);

    Keyframe(float duration, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, bool easeIn = false, bool easeOut = false);
};

#endif //PPGSO_KEYFRAME_H