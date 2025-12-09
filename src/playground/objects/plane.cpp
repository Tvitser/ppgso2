#include "plane.h"

#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>

std::unique_ptr<ppgso::Mesh> Plane::mesh;
std::unique_ptr<ppgso::Shader> Plane::shader;
std::unique_ptr<ppgso::Texture> Plane::texture;

Plane::Plane(Object* parent) {
    parentObject = parent;
    position = {0, 0, 0};
    rotation = {0, 0, 0};
    scale = {1, 1, 1};

    if (!mesh) {
        mesh = std::make_unique<ppgso::Mesh>("objects/ground.obj");
    }
    if (!shader) {
        shader = std::make_unique<ppgso::Shader>(phong_vert_glsl, phong_frag_glsl);
    }
    if (!texture) {
        texture = std::make_unique<ppgso::Texture>(ppgso::image::loadBMP("textures/ground.bmp"));
    }
}

bool Plane::update(Scene &scene, float, glm::mat4 parentModelMatrix, glm::vec3) {
    generateModelMatrix(parentModelMatrix);
    return true;
}

void Plane::render(Scene &scene, GLuint depthMap) {
    shader->use();

    // Use correct uniform names matching phong shader
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

    shader->setUniform("Texture", *texture);

    // Shadow maps are already bound by SceneWindow to texture units 1-4
    shader->setUniform("shadowMap0", 1);
    shader->setUniform("shadowMap1", 2);
    shader->setUniform("shadowMap2", 3);
    shader->setUniform("shadowMap3", 4);

    scene.renderLight(shader, true);

    mesh->render();
}

void Plane::renderForShadow(Scene &scene) {
    generateModelMatrix(parentObject ? parentObject->modelMatrix : glm::mat4{1.0f});

    // Use the currently bound shadow shader from SceneWindow (don't call shader_shadow->use())
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

    mesh->render();
}
