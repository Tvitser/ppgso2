#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <shader.h>

#include "object.h"

/**
 * @param scene
 */
bool Object::updateChildren(Scene &scene, float time, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) {
    bool remove = this->update(scene, time, parentModelMatrix, parentRotation);
    globalRotation = parentRotation + rotation;

    // Use iterator to update all objects so we can remove while iterating
    auto i = std::begin(childObjects);

    while (i != std::end(childObjects)) {
        // Update and remove from list if needed
        auto obj = i->get();
        if (!obj->updateChildren(scene, time, modelMatrix, globalRotation))
            i = childObjects.erase(i); // NOTE: no need to call destructors as we store shared pointers in the scene
        else
            ++i;
    }
    return remove;
}

/**
 * @param scene
 */
void Object::renderChildren(Scene &scene, GLuint depthMap) {
    this->render(scene, depthMap);
    for ( auto& obj : childObjects )
        obj->renderChildren(scene, depthMap);
}

/**
 * @param scene
 */
void Object::renderForShadowChildren(Scene &scene) {
    this->renderForShadow(scene);
    for ( auto& obj : childObjects )
        obj->renderForShadowChildren(scene);
}

void Object::generateModelMatrix(glm::mat4 parentModelMatrix) {
    modelMatrix = parentModelMatrix
            * glm::translate(glm::mat4(1.0f), position)
            * glm::orientate4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);
}
