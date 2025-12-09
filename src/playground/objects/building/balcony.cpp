//
// Created by Pavel on 29.11.2025.
//
#include <iostream>
#include "balcony.h"
#include <filesystem>

//
// Created by Pavel on 25.11.2025.
//

#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>


std::unique_ptr<ppgso::Mesh> Balcony::mesh;
std::unique_ptr<ppgso::Shader> Balcony::shader;
std::unique_ptr<ppgso::Texture> Balcony::texture;
static inline glm::mat3 makeNormalMatrix(const glm::mat4& model, const glm::mat4& view) {
    glm::mat4 mv = view * model;
    return glm::transpose(glm::inverse(glm::mat3(mv)));
}
Balcony::Balcony(Object* parent) {
    parentObject = parent;
    position = {0, 0, 0};
    rotation = {0, 0, 0};
    scale = {1, 1, 1};
    if (!mesh) {
        mesh = std::make_unique<ppgso::Mesh>("objects/building/Building_Balconies.obj");
    }
    if (!shader) {
        shader = std::make_unique<ppgso::Shader>(phong_vert_glsl, phong_frag_glsl);
    }
    if (!texture) {
        texture = std::make_unique<ppgso::Texture>(ppgso::image::loadBMP("textures/building/Balconies.bmp"));
    }
}
bool Balcony::update(Scene &scene, float dt,
                      glm::mat4 parentModelMatrix,
                      glm::vec3 parentRotation) {
    generateModelMatrix(parentModelMatrix);
    return true;
}

void Balcony::render(Scene &scene, GLuint depthMap) {
    shader->use();

    // соответствие имен uniform тем, что в phong_vert.glsl / phong_frag_glsl
    shader->setUniform("projection", scene.camera->projectionMatrix);
    shader->setUniform("view", scene.camera->viewMatrix);
    shader->setUniform("model", modelMatrix);

    // Set multiple light space matrices
    shader->setUniform("numShadowMaps", scene.numShadowMaps);
    for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
        std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, scene.lightSpaceMatrices[i]);
    }

    // Основная текстура: setUniform сам привязывает её к слоту 0 и устанавливает sampler
    shader->setUniform("Texture", *texture);

    // Shadow maps are already bound by SceneWindow to texture units 1-4
    shader->setUniform("shadowMap0", 1);
    shader->setUniform("shadowMap1", 2);
    shader->setUniform("shadowMap2", 3);
    shader->setUniform("shadowMap3", 4);
    shader->setUniform("pointShadowMaps[0]", 5);
    shader->setUniform("pointShadowMaps[1]", 6);

    // установить параметры света (функция сцены должна выставлять глобальные/спот/прочие uniforms)
    scene.renderLight(shader, true);

    mesh->render();
}

void Balcony::renderForShadow(Scene &scene) {
    // Общий shadow-шейдер уже активен в PASS 1
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint locModel = glGetUniformLocation(static_cast<GLuint>(currentProgram), "ModelMatrix");
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
    if (mesh) mesh->render();
}

void Balcony::renderForShadow(Scene &scene, GLuint shadowProgram) {
    // Оставляем для совместимости, но SceneWindow использует renderForShadow(Scene&)
    GLint locModel = glGetUniformLocation(shadowProgram, "ModelMatrix");
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
    if (mesh) mesh->render();
}
