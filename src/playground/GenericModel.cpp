#include "GenericModel.hpp"
#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <shader/texture_vert_glsl.h>
#include <shader/texture_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

std::unordered_map<std::string, std::shared_ptr<ppgso::Mesh>> GenericModel::meshCache;
std::unordered_map<std::string, std::shared_ptr<ppgso::Texture>> GenericModel::texCache;
std::unique_ptr<ppgso::Shader> GenericModel::shader = nullptr;
std::unique_ptr<ppgso::Shader> GenericModel::textureShader = nullptr;

GenericModel::GenericModel(Object* parent, const std::string &meshFile, const std::string &texFile) {
    parentObject = parent;
    meshPath = meshFile;
    texturePath = texFile;
    scale = {1,1,1};
    rotation = {0,0,0};
    position = {0,0,0};
    ensureResources();
}

void GenericModel::ensureResources() {
    if (!shader) shader = std::make_unique<ppgso::Shader>(phong_vert_glsl, phong_frag_glsl);
    if (!textureShader) textureShader = std::make_unique<ppgso::Shader>(texture_vert_glsl, texture_frag_glsl);

    if (meshCache.find(meshPath) == meshCache.end()) {
        meshCache[meshPath] = std::make_shared<ppgso::Mesh>(meshPath);
    }
    if (!texturePath.empty() && texCache.find(texturePath) == texCache.end()) {
        texCache[texturePath] = std::make_shared<ppgso::Texture>(ppgso::image::loadBMP(texturePath));
    }
}

void GenericModel::enableInstancing(int count, float radius, int seed) {
    constexpr int MIN_INSTANCE_COUNT = 1;
    instanced = true;
    instanceCount = std::max(MIN_INSTANCE_COUNT, count);
    instanceRadius = radius;
    instanceSeed = seed;
}

void GenericModel::useTextureShader() {
    useTexture = true;
}

void GenericModel::disableShadows() {
    castShadows = false;
}

bool GenericModel::update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) {
    generateModelMatrix(parentModelMatrix);
    return true;
}

void GenericModel::render(Scene &scene, GLuint depthMap) {
    // Активируем шейдер и устанавливаем общие матрицы/текстуры
    std::unique_ptr<ppgso::Shader> &activeShader = useTexture ? textureShader : shader;
    activeShader->use();
    if (useTexture) {
        activeShader->setUniform("ProjectionMatrix", scene.camera->projectionMatrix);
        activeShader->setUniform("ViewMatrix", scene.camera->viewMatrix);
        activeShader->setUniform("ModelMatrix", modelMatrix);
    } else {
        activeShader->setUniform("projection", scene.camera->projectionMatrix);
        activeShader->setUniform("view", scene.camera->viewMatrix);
        activeShader->setUniform("model", modelMatrix);
    }

    activeShader->setUniform("useInstancing", instanced ? 1 : 0);
    activeShader->setUniform("instanceRadius", instanced ? instanceRadius : 0.0f);
    activeShader->setUniform("instanceSeed", instanceSeed);

    if (!texturePath.empty() && texCache.count(texturePath)) {
        activeShader->setUniform("Texture", *texCache[texturePath]);
    }
    if (useTexture) {
        activeShader->setUniform("TextureOffset", glm::vec2{0.0f, 0.0f});
    }

    if (!useTexture) {
        // Set multiple light space matrices
        activeShader->setUniform("numShadowMaps", scene.numShadowMaps);
        for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
            std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
            activeShader->setUniform(uniformName, scene.lightSpaceMatrices[i]);
        }

        // Shadow maps are already bound by SceneWindow to texture units 1-4
        activeShader->setUniform("shadowMap0", 1);
        activeShader->setUniform("shadowMap1", 2);
        activeShader->setUniform("shadowMap2", 3);
        activeShader->setUniform("shadowMap3", 4);
        activeShader->setUniform("pointShadowMaps[0]", 5);
        activeShader->setUniform("pointShadowMaps[1]", 6);

        // scene.renderLight перезаписывает много uniform'ов (включая Transparency),
        // поэтому устанавливаем Transparency после него
        scene.renderLight(activeShader, true);
    }

    // Устанавливаем прозрачность ПОСЛЕ renderLight / перед рендером
    float transp = transparent ? 0.25f : 1.0f;
    activeShader->setUniform("Transparency", transp);

    auto meshIt = meshCache.find(meshPath);
    if (meshIt != meshCache.end()) {
        if (instanced && instanceCount > 0) {
            meshIt->second->renderInstanced(instanceCount);
        } else {
            meshIt->second->render();
        }
    }
}

void GenericModel::renderForShadow(Scene &scene) {
    if (!castShadows) return;
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    GLint locUseInst = glGetUniformLocation(static_cast<GLuint>(currentProgram), "useInstancing");
    if (locUseInst >= 0) glUniform1i(locUseInst, instanced ? 1 : 0);
    GLint locRadius = glGetUniformLocation(static_cast<GLuint>(currentProgram), "instanceRadius");
    if (locRadius >= 0) glUniform1f(locRadius, instanced ? instanceRadius : 0.0f);
    GLint locSeed = glGetUniformLocation(static_cast<GLuint>(currentProgram), "instanceSeed");
    if (locSeed >= 0) glUniform1i(locSeed, instanceSeed);

    GLint locModel = glGetUniformLocation(static_cast<GLuint>(currentProgram), "ModelMatrix");
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
    auto meshIt = meshCache.find(meshPath);
    if (meshIt != meshCache.end()) {
        if (instanced && instanceCount > 0) {
            meshIt->second->renderInstanced(instanceCount);
        } else {
            meshIt->second->render();
        }
    }
}

void GenericModel::renderForShadow(Scene &scene, GLuint) {
    renderForShadow(scene);
}

// Пустая реализация столкновений
void GenericModel::checkCollisions(Scene &scene, float dt) {
    // no-op
}
