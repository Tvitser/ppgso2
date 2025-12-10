#include "GenericModel.hpp"
#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>

std::unordered_map<std::string, std::shared_ptr<ppgso::Mesh>> GenericModel::meshCache;
std::unordered_map<std::string, std::shared_ptr<ppgso::Texture>> GenericModel::texCache;
std::unique_ptr<ppgso::Shader> GenericModel::shader = nullptr;

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

    if (meshCache.find(meshPath) == meshCache.end()) {
        meshCache[meshPath] = std::make_shared<ppgso::Mesh>(meshPath);
    }
    if (!texturePath.empty() && texCache.find(texturePath) == texCache.end()) {
        texCache[texturePath] = std::make_shared<ppgso::Texture>(ppgso::image::loadBMP(texturePath));
    }
}

bool GenericModel::update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) {
    if (!keyframes.empty() && !keyframesOver) {
        age += dt;
        keyframesUpdate(scene);
    }
    glm::mat4 localModel = Object::getModelMatrix(position, rotation, scale);
    modelMatrix = parentModelMatrix * localModel;
    return true;
}

void GenericModel::render(Scene &scene, GLuint depthMap) {
    // Активируем шейдер и устанавливаем общие матрицы/текстуры
    shader->use();
    shader->setUniform("projection", scene.camera->projectionMatrix);
    shader->setUniform("view", scene.camera->viewMatrix);
    shader->setUniform("model", modelMatrix);

    // Set multiple light space matrices
    shader->setUniform("numShadowMaps", scene.numShadowMaps);
    for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
        std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, scene.lightSpaceMatrices[i]);
    }

    if (!texturePath.empty() && texCache.count(texturePath)) {
        shader->setUniform("Texture", *texCache[texturePath]);
    }

    // Shadow maps are already bound by SceneWindow to texture units 1-4
    shader->setUniform("shadowMap0", 1);
    shader->setUniform("shadowMap1", 2);
    shader->setUniform("shadowMap2", 3);
    shader->setUniform("shadowMap3", 4);
    shader->setUniform("pointShadowMaps[0]", 5);
    shader->setUniform("pointShadowMaps[1]", 6);

    // scene.renderLight перезаписывает много uniform'ов (включая Transparency),
    // поэтому устанавливаем Transparency после него
    scene.renderLight(shader, true);

    // Устанавливаем прозрачность ПОСЛЕ renderLight
    float transp = transparent ? 0.25f : 1.0f;
    shader->setUniform("Transparency", transp);

    if (meshCache.count(meshPath)) meshCache[meshPath]->render();
}

void GenericModel::renderForShadow(Scene &scene) {
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint locModel = glGetUniformLocation(static_cast<GLuint>(currentProgram), "ModelMatrix");
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }
    if (meshCache.count(meshPath)) meshCache[meshPath]->render();
}

void GenericModel::renderForShadow(Scene &scene, GLuint) {
    renderForShadow(scene);
}

// Пустая реализация столкновений
void GenericModel::checkCollisions(Scene &scene, float dt) {
    // no-op
}
