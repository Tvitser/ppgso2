#include <glm/glm.hpp>

#include "camera.h"
#include "object.h"

Camera::Camera(float fow, float ratio, float near, float far) {
    float fowInRad = (ppgso::PI/180.0f) * fow;

    projectionMatrix = glm::perspective(fowInRad, ratio, near, far);
}

void Camera::update(float time) {
    age += time;
    if (!keyframes.empty() && useKeyframes) {
        float t = 0.0f;
        glm::vec3 actualpos, nextpos, lastpos;
        glm::vec3 actualrot, nextrot, lastrot;
        bool easeIn, easeOut;
        float duration;
        bool flag = false;
        for (auto iter= keyframes.begin(); iter != keyframes.end(); iter++) {
            if (t + iter->duration > age) {
                actualpos = iter->position;
                actualrot = iter->rotation;
                duration = iter->duration;
                easeIn = iter->easeIn;
                easeOut = iter->easeOut;
                iter++;
                if (iter == keyframes.end()) {
                    position = actualpos;
                    tilt = actualrot[0];
                    rotation = actualrot[1];
                    flag = true;
                    break;
                }
                nextpos = iter->position;
                nextrot = iter->rotation;
                float a = (age - t) / duration;
                a = Object::mapTime(a, easeIn, easeOut);
                position = glm::lerp(actualpos, nextpos, a);
                glm::vec3 r = glm::lerp(actualrot, nextrot, a);
                tilt = r[0];
                rotation = r[1];
                flag = true;
                break;
            }
            lastpos = iter->position;
            lastrot = iter->rotation;
            t += iter->duration;
        }
        if (!flag) {
            position = lastpos;
            tilt = lastrot[0];
            rotation = lastrot[1];
        }
    }
    viewMatrix = glm::mat4{1.0f}
                 * glm::rotate(glm::mat4{1.0f}, (ppgso::PI / 180.0f) * tilt, {1, 0, 0})
                 * glm::rotate(glm::mat4{1.0f}, (ppgso::PI / 180.0f) * rotation, {0, 1, 0})
                 * glm::translate(glm::mat4{1.0f}, -position);
}

void Camera::moveX(int dir) {
    position.x += dir * .5;
    debug();
}
void Camera::moveY(int dir) {
    position.y += dir * .5;
    debug();
}
void Camera::moveZ(int dir) {
    position.z += dir * .5;
    debug();
}
void Camera::rotate(int dir) {
    rotation += dir * 1.5;
    debug();
}

void Camera::debug() {
    if (debugEnabled) {
        std::cout << "Debug: Camera position " << position.x << " " << position.y << " " << position.z << " " << rotation << std::endl;
    }
}