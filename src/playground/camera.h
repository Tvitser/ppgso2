#ifndef PPGSO_CAMERA_H
#define PPGSO_CAMERA_H

#include <memory>

#include <glm/glm.hpp>
#include <../ppgso/ppgso.h>
#include <list>
#include "keyframe.h"

class Camera {
public:
    glm::vec3 position{0,0,0};
    bool flag = true;
    float tilt = 0.f;
    float rotation = 0.f;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    float age = 0.0f;

    bool debugEnabled = false;
    bool useKeyframes = true;

    /*!
     * Create new Camera that will generate viewMatrix and projectionMatrix based on its position, up and back vectors
     * @param fow - Field of view in degrees
     * @param ratio - Viewport screen ratio (usually width/height of the render window)
     * @param near - Distance to the near frustum plane
     * @param far - Distance to the far frustum plane
     */
    explicit Camera(float fow = 45.0f, float ratio = 1.0f, float near = 0.1f, float far = 10.f);

    /*!
     * Update Camera viewMatrix based on up, position and back vectors
     */
    void update(float time);

    void moveX(int dir);
    void moveY(int dir);
    void moveZ(int dir);

    void rotate(int dir);

    void debug();

    static glm::mat4 getViewMatrix(float tilt, float rotation, glm::vec3 position) {
        return glm::mat4{1.0f}
        * glm::rotate(glm::mat4{1.0f}, (ppgso::PI/180.0f) * tilt, {1, 0, 0})
        * glm::rotate(glm::mat4{1.0f}, (ppgso::PI/180.0f) * rotation, {0, 1, 0})
        * glm::translate(glm::mat4{1.0f}, -position);
    }

    std::list<Keyframe> keyframes;
};

#endif //PPGSO_CAMERA_H