#include "scene.h"
#include <vector>
#include <algorithm>

constexpr int MAX_LIGHTS = 10;
constexpr glm::vec3 LIGHT_AMBIENT_INTENSITY{0.1f};
constexpr glm::vec3 LIGHT_DIFFUSE_INTENSITY{0.6f};
constexpr glm::vec3 LIGHT_SPECULAR_INTENSITY{0.3f};
constexpr glm::vec3 DEFAULT_MATERIAL_AMBIENT{0.2f};
constexpr glm::vec3 DEFAULT_MATERIAL_DIFFUSE{0.8f};
constexpr glm::vec3 DEFAULT_MATERIAL_SPECULAR{0.5f};

// Вспомогательная рекурсивная функция:
// собирает ВСЕ объекты сцены (включая детей) в два списка – opaque и transparent.
namespace {
    void collectObjects(Object* obj,
                        std::vector<Object*>& opaque,
                        std::vector<Object*>& transparentObjects) {
        if (!obj) return;

        if (obj->transparent)
            transparentObjects.push_back(obj);
        else
            opaque.push_back(obj);

        // Рекурсивно пройтись по дочерним объектам
        for (auto& child : obj->childObjects) {
            collectObjects(child.get(), opaque, transparentObjects);
        }
    }
}



void Scene::update(float time) {
    if (camera) camera->update(time);

    for (auto &o: rootObjects) o->checkCollisions(*this, time);

    auto i = std::begin(rootObjects);
    while (i != std::end(rootObjects)) {
        auto obj = i->get();
        if (!obj->updateChildren(*this, time, glm::mat4{1.0f}, {0, 0, 0}))
            i = rootObjects.erase(i);
        else
            ++i;
    }
}

void Scene::render(GLuint depthMaps[MAX_SHADOW_MAPS], int numMaps) {
    // Собираем ВСЕ объекты (включая детей) в два списка
    std::vector<Object*> opaque;
    std::vector<Object*> transparentObjects;

    opaque.reserve(rootObjects.size() * 4);
    transparentObjects.reserve(rootObjects.size() * 4);

    for (auto& up : rootObjects) {
        collectObjects(up.get(), opaque, transparentObjects);
    }

    // --- Непрозрачные: depth write ON, blending OFF ---
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    for (auto* o : opaque) {
        o->render(*this, depthMaps[0]); // Pass first shadow map for backward compatibility
    }

    // --- Прозрачные: сортируем от дальних к ближним ---
    if (!transparentObjects.empty()) {
        const glm::vec3 camPos = camera ? camera->position : glm::vec3(0.0f);

        // Правильная сортировка по расстоянию до камеры
        std::sort(transparentObjects.begin(), transparentObjects.end(),
                  [&camPos](Object* a, Object* b) {
                      float distA = glm::length(camPos - glm::vec3(a->modelMatrix[3]));
                      float distB = glm::length(camPos - glm::vec3(b->modelMatrix[3]));
                      return distA > distB; // дальние сначала
                  });

        // Настройка blending для прозрачности
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Отключаем запись в буфер глубины для прозрачных объектов

        // Рендер прозрачных объектов
        for (auto* o : transparentObjects) {
            o->render(*this, depthMaps[0]);
        }

        // Восстанавливаем настройки
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

// language: cpp
// Заменить реализацию Scene::renderLight в `src/playground/scene.cpp`
void Scene::renderLight(std::unique_ptr<ppgso::Shader> &shader, bool) {
    shader->use();

    std::vector<Light*> activeLights;
    activeLights.reserve(lights.size() + 1);
    bool hasMainLight = false;
    for (auto* light : lights) {
        if (!light) continue;
        if (mainlight && light == mainlight.get()) hasMainLight = true;
        activeLights.push_back(light);
    }
    if (mainlight && !hasMainLight) {
        activeLights.push_back(mainlight.get());
    }

    int count = static_cast<int>(std::min<size_t>(activeLights.size(), static_cast<size_t>(MAX_LIGHTS)));
    shader->setUniform("numberOfLights", count);
    
    // Set number of shadow maps and shadow caster indices
    shader->setUniform("numShadowMaps", numShadowMaps);
    shader->setUniform("numPointShadowMaps", numPointShadowMaps);
    for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
        std::string uniformName = "shadowCasterIndices[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, i < numShadowMaps ? shadowCasterIndices[i] : -1);
    }
    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        std::string uniformName = "pointShadowCasterIndices[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, i < numPointShadowMaps ? pointShadowCasterIndices[i] : -1);
        uniformName = "pointShadowFarPlane[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, i < numPointShadowMaps ? pointShadowFarPlane[i] : 0.0f);
    }

    // Set light space matrices for all shadow-casting lights
    for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
        std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, i < numShadowMaps ? lightSpaceMatrices[i] : glm::mat4(1.0f));
    }

    for (int i = 0; i < count; ++i) {
        auto* L = activeLights[i];
        const std::string idx = "lights[" + std::to_string(i) + "]";
        auto dir = L->effectiveDirection();

        shader->setUniform(idx + ".type", static_cast<int>(L->type));
        shader->setUniform(idx + ".position", L->position);
        shader->setUniform(idx + ".direction", dir);
        shader->setUniform(idx + ".cutOff", L->cutOff);
        shader->setUniform(idx + ".outerCutOff", L->outerCutOff);
        shader->setUniform(idx + ".color", L->color);
        shader->setUniform(idx + ".ambient", LIGHT_AMBIENT_INTENSITY);
        shader->setUniform(idx + ".diffuse", LIGHT_DIFFUSE_INTENSITY);
        shader->setUniform(idx + ".specular", LIGHT_SPECULAR_INTENSITY);
        shader->setUniform(idx + ".constant", L->constant);
        shader->setUniform(idx + ".linear", L->linear);
        shader->setUniform(idx + ".quadratic", L->quadratic);
        shader->setUniform(idx + ".maxDist", L->maxDist);
    }

    // камера и общие параметры
    if (camera) shader->setUniform("viewPos", camera->position);

    shader->setUniform("Transparency", 1.0f);
    shader->setUniform("textureOffset", glm::vec2{0.0f, 0.0f});

    // материал по умолчанию (объекты могут переопределять)
    shader->setUniform("material.ambient",  DEFAULT_MATERIAL_AMBIENT);
    shader->setUniform("material.diffuse",  DEFAULT_MATERIAL_DIFFUSE);
    shader->setUniform("material.specular", DEFAULT_MATERIAL_SPECULAR);
    shader->setUniform("material.shininess", 32.0f);
}


void Scene::renderForShadow(GLuint depthMap) {
    for ( auto& obj : rootObjects )
        obj->renderForShadowChildren(*this);
}

void Scene::close() {
    rootObjects.clear();
    lights.clear();
    mainlight = nullptr;
}
