// File: src/playground/GenericModel.hpp
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <ppgso/ppgso.h>
#include "object.h"
#include "scene.h"

class Group : public Object {
public:
    Group(Object* parent = nullptr) { parentObject = parent; }
    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) override {
        generateModelMatrix(parentModelMatrix);
        return true;
    }
    void render(Scene &scene, GLuint depthMap) override { /* пусто */ }
    void renderForShadow(Scene &scene) override { /* пусто */ }
    void renderForShadow(Scene &scene, GLuint) { /* пусто */ }
    void checkCollisions(Scene &scene, float dt) override { /* пусто */ };
};

class GenericModel : public Object {
public:
    GenericModel(Object* parent, const std::string &meshFile, const std::string &texFile);

    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) override;
    void render(Scene &scene, GLuint depthMap) override;
    void renderForShadow(Scene &scene) override;
    void renderForShadow(Scene &scene, GLuint);

    // Добавлено: реализация чисто-виртуального метода базового класса
    void checkCollisions(Scene &scene, float dt) override;

    // Enable hardware instancing for this model
    void enableInstancing(int count, float radius, int seed);
private:
    std::string meshPath;
    std::string texturePath;

    // Кэш мешей/текстур/шейдера
    static std::unordered_map<std::string, std::shared_ptr<ppgso::Mesh>> meshCache;
    static std::unordered_map<std::string, std::shared_ptr<ppgso::Texture>> texCache;
    static std::unique_ptr<ppgso::Shader> shader;

    void ensureResources();

    bool instanced = false;
    int instanceCount = 1;
    float instanceRadius = 0.f;
    int instanceSeed = 0;
};
