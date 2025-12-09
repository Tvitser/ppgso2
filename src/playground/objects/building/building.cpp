//
// Created by Pavel on 25.11.2025.
//
#include "building.h"

#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>


std::unique_ptr<ppgso::Mesh> Building::mesh;
std::unique_ptr<ppgso::Shader> Building::shader;
std::unique_ptr<ppgso::Texture> Building::texture;
static inline glm::mat3 makeNormalMatrix(const glm::mat4& model, const glm::mat4& view) {
    glm::mat4 mv = view * model;
    return glm::transpose(glm::inverse(glm::mat3(mv)));
}
Building::Building(Object* parent) {
    parentObject = parent;
    position = {0, 0, 0};
    rotation = {0, 0, 0};
    scale = {1, 1, 1};

    if (!mesh) {
        mesh = std::make_unique<ppgso::Mesh>("objects/building/Building_base.obj");
    }
    if (!shader) {
        shader = std::make_unique<ppgso::Shader>(phong_vert_glsl, phong_frag_glsl);
    }
    if (!texture) {
        texture = std::make_unique<ppgso::Texture>(ppgso::image::loadBMP("textures/atlas_building_base.bmp"));
    }
}
bool Building::update(Scene &scene, float dt,
                      glm::mat4 parentModelMatrix,
                      glm::vec3 parentRotation) {
    generateModelMatrix(parentModelMatrix);
    return true;
}

void Building::render(Scene &scene, GLuint depthMap) {
    shader->use();

    // соответствие имен uniform тем, что в phong_vert.glsl / phong_frag.glsl
    shader->setUniform("projection", scene.camera->projectionMatrix);
    shader->setUniform("view", scene.camera->viewMatrix);
    shader->setUniform("model", modelMatrix);

    shader->setUniform("useInstancing", 0);
    shader->setUniform("instanceRadius", 0.0f);
    shader->setUniform("instanceSeed", 0);

    // Set multiple light space matrices
    shader->setUniform("numShadowMaps", scene.numShadowMaps);
    for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
        std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, scene.lightSpaceMatrices[i]);
    }

    // Основная текстура в слоте 0 (привязка выполняется внутри setUniform)
    shader->setUniform("Texture", *texture);

    // Shadow maps are already bound by SceneWindow to texture units 1-4
    shader->setUniform("shadowMap0", 1);
    shader->setUniform("shadowMap1", 2);
    shader->setUniform("shadowMap2", 3);
    shader->setUniform("shadowMap3", 4);

    // установить параметры света (функция сцены должна выставлять глобальные/спот/прочие uniforms)
    scene.renderLight(shader, true);

    mesh->render();
}

void Building::renderForShadow(Scene &scene) {
    // Общий shadow-шейдер уже активен в PASS 1
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint locUseInst = glGetUniformLocation(static_cast<GLuint>(currentProgram), "useInstancing");
    if (locUseInst >= 0) glUniform1i(locUseInst, 0);
    GLint locRadius = glGetUniformLocation(static_cast<GLuint>(currentProgram), "instanceRadius");
    if (locRadius >= 0) glUniform1f(locRadius, 0.0f);
    GLint locSeed = glGetUniformLocation(static_cast<GLuint>(currentProgram), "instanceSeed");
    if (locSeed >= 0) glUniform1i(locSeed, 0);
    GLint locModel = glGetUniformLocation(static_cast<GLuint>(currentProgram), "ModelMatrix");
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
    if (mesh) mesh->render();
}

void Building::renderForShadow(Scene &scene, GLuint shadowProgram) {
    GLint locModel = glGetUniformLocation(shadowProgram, "ModelMatrix");
    GLint locUseInst = glGetUniformLocation(shadowProgram, "useInstancing");
    if (locUseInst >= 0) glUniform1i(locUseInst, 0);
    GLint locRadius = glGetUniformLocation(shadowProgram, "instanceRadius");
    if (locRadius >= 0) glUniform1f(locRadius, 0.0f);
    GLint locSeed = glGetUniformLocation(shadowProgram, "instanceSeed");
    if (locSeed >= 0) glUniform1i(locSeed, 0);
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
    if (mesh) mesh->render();
}
