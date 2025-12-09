#ifndef PPGSO_MAINLIGHT_H
#define PPGSO_MAINLIGHT_H

#include "object.h"
#include "light.h"
#include "glm/gtc/matrix_transform.hpp"

class MainLight : public Light {
public:
    MainLight() = default;

    MainLight(glm::vec3 color, float constant, float linear, float quadratic, float maxDist);
    MainLight(glm::vec3 color, glm::vec3 direction, float cutOff, float outerCutOff,
              float constant, float linear, float quadratic, float maxDist);

    // Параметры для карты теней
    float near_plane = 0.5f;
    float far_plane  = 500.0f;
    // половины размеров ортографического объёма (можно менять для масштабирования карты теней)
    float orthoHalfWidth  = 50.0f;
    float orthoHalfHeight = 50.0f;

    // Проекция света для теней — пересчитывается вызовом updateProjection()
    glm::mat4 lightProjectionMatrix = glm::mat4(1.0f);

    // Видовая матрица света (используется SceneWindow как getLightView())
    glm::mat4 getLightView() const {
        return glm::lookAt(
            position,                   // позиция света
            glm::vec3(0.0f, 0.0f, 0.0f),// центр сцены (можно изменить при необходимости)
            glm::vec3(0.0f, 1.0f, 0.0f) // up
        );
    }

    // Пересчёт ортографической проекции (вызывать при изменении orthoHalfWidth/Height или near/far)
    void updateProjection() {
        // Для directional света используем орто-проекцию, покрывающую сцену
        float orthoSize = 150.0f;
        lightProjectionMatrix = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, maxDist);
        // Если у вас кастомный getLightView() — убедитесь, что direction ненулевой и нормализован
    }
};

#endif //PPGSO_MAINLIGHT_H
