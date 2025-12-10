#ifndef PPGSO_SCENEWINDOW_HPP
#define PPGSO_SCENEWINDOW_HPP

#include <utility>
#include <map>
#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <array>
#include <ppgso/shader.h>
#include <ppgso/ppgso.h>
#include <filesystem>
#include "GenericModel.hpp"
#include <shader/texture_vert_glsl.h>
#include <shader/texture_frag_glsl.h>

#include "camera.h"
#include "scene.h"
#include "light.h"
#include "keyframe.h"

// === Базовые объекты ===
#include "objects/plane.h"

#include "objects/building/balcony.h"
#include "objects/building/building.h"
#include "objects/skybox.h"

#include <glm/gtc/type_ptr.hpp>

// Включаем сгенерированные shadow шейдеры
#include <shader/shadow_vert_glsl.h>
#include <shader/shadow_frag_glsl.h>
namespace fs = std::filesystem;
class SceneWindow : public ppgso::Window {
private:
    Scene scene;
    bool animate = true;
    const float fow = 60.0f;
    float ratio = 1.0f;
    float loadTime = -1.f;

    int size_x, size_y;

    // Shadow mapping - support for multiple shadow maps
    static const int NUM_SHADOW_MAPS = 4;
    GLuint shadowMapFBOs[NUM_SHADOW_MAPS] = {0};
    GLuint shadowMaps[NUM_SHADOW_MAPS] = {0};
    const int SHADOW_SIZE = 2048;
    static const int NUM_POINT_SHADOW_MAPS = MAX_POINT_SHADOW_MAPS;
    GLuint pointShadowMapFBOs[NUM_POINT_SHADOW_MAPS] = {0};
    GLuint pointShadowMaps[NUM_POINT_SHADOW_MAPS] = {0};
    const int POINT_SHADOW_SIZE = SHADOW_SIZE;
    // Общий шейдер для рендера теней (depth)
    std::unique_ptr<ppgso::Shader> shadowShader;

    // Camera & input speeds
    float camMoveSpeed  = 3.0f;
    float camRotSpeed   = 90.0f;
    float lampMoveSpeed = 3.0f;

    Skybox* skybox = nullptr;

    // Light objects
    Light pointLightA;
    Light spotLight;
    Light pointLightB;

    // RNG
    std::mt19937 rng;

    // Mouse control
    bool   firstMouse       = true;
    double lastX            = 0.0;
    double lastY            = 0.0;
    float  mouseSensitivity = 0.15f;

    // Helper function to compute light space matrix for a point light (looking at scene center)
    glm::mat4 computePointLightSpaceMatrix(const Light& light) {
        // For point lights, we create a perspective projection looking at scene center
        float near = 0.1f;
        float far = light.maxDist > 0 ? light.maxDist : 100.0f;
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, near, far);
        glm::mat4 view = glm::lookAt(light.position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        return proj * view;
    }

    // Helper function to compute light space matrix for a spot light
    glm::mat4 computeDirectionalLightSpaceMatrix(const Light& light) {
        float orthoSize = 150.0f;
        glm::mat4 proj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 500.0f);
        // For spot light, position it far along -direction
        glm::vec3 lightDir = light.effectiveDirection();
        if (glm::length(lightDir) < 0.001f) lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 lightPos = -lightDir * 200.0f;
        glm::mat4 view = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        return proj * view;
    }

    std::array<glm::mat4, 6> buildPointShadowTransforms(const glm::vec3& lightPos, float nearPlane, float farPlane) {
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
        // Cube faces: +X, -X, +Y, -Y, +Z, -Z
        return {
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0))
        };
    }

    void createShadowResources() {
        // Create multiple shadow map framebuffers and textures
        glGenFramebuffers(NUM_SHADOW_MAPS, shadowMapFBOs);
        glGenTextures(NUM_SHADOW_MAPS, shadowMaps);

        float clampColor[4] = {1.f, 1.f, 1.f, 1.f};

        for (int i = 0; i < NUM_SHADOW_MAPS; ++i) {
            glBindTexture(GL_TEXTURE_2D, shadowMaps[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                         SHADOW_SIZE, SHADOW_SIZE, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clampColor);

            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBOs[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, shadowMaps[i], 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void createPointShadowResources() {
        glGenFramebuffers(NUM_POINT_SHADOW_MAPS, pointShadowMapFBOs);
        glGenTextures(NUM_POINT_SHADOW_MAPS, pointShadowMaps);

        for (int i = 0; i < NUM_POINT_SHADOW_MAPS; ++i) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowMaps[i]);
            for (int face = 0; face < 6; ++face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT,
                             POINT_SHADOW_SIZE, POINT_SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, pointShadowMapFBOs[i]);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointShadowMaps[i], 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    // === Add table with random chairs and glasses ===

    void initScene() {
        scene.rootObjects.clear();

        // === Main light ===
        glm::vec3 lightPos  = glm::vec3(20.f, 50.f, 0.f);
        glm::vec3 lightTarget = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 lightDir = glm::normalize(lightTarget - lightPos);

        // Создаём mainlight как направленный (direction) свет
        scene.mainlight = std::make_unique<MainLight>(
            glm::vec3(1.0f, 1.0f, 1.0f), // color
            lightDir,                   // direction (normalized)
            0.0f, 0.0f,                 // cutOff, outerCutOff (неважно для spot)
            1.0f, 1.0f, 0.0f, 700.0f    // attenuation params + maxDist
        );

        scene.mainlight->position = lightPos;
        scene.mainlight->localDirection = glm::normalize(glm::vec3(-1, -1, -1)); // << НОВОЕ!
        // Явно задать орто-проекцию для spot света (покрыть сцену)
        float orthoSize = 500.0f;
        scene.mainlight->lightProjectionMatrix = glm::ortho(
            -orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 600.0f
        );

        // Обновить внутренние данные (если метод есть)
        scene.mainlight->updateProjection();

        scene.lights.clear();
        scene.lights.push_back(scene.mainlight.get());

        pointLightA = Light(glm::vec3(1.0f), 1.0f, 0.01f, 0.01f, 100.0f);
        pointLightA.position = glm::vec3(-0.425795f, 1.63406f, 1.51299f);
        scene.lights.push_back(&pointLightA);

        // Direction vector derived from the requested spot light coordinates.
        spotLight.position = glm::vec3(-4.85891, 1.27937, 2.68917);
        spotLight = Light(glm::vec3(1.f,0.f,1.f), glm::normalize(glm::vec3(-4.85891, 0.27937, 2.68917)-spotLight.position),
                                 15.f, 18.f, 1.0f, 0.79f, 0.032f, 1.3f);
        spotLight.position = glm::vec3(-4.85891, 1.27937, 2.68917);
        spotLight.type=LightType::Spot;
        scene.lights.push_back(&spotLight);

        pointLightB = Light(glm::vec3(0.0,1.f,0.f), 1.0f, 0.09f, 0.032f, 4.0f);
        pointLightB.position = glm::vec3(13.7437f, 0.772186f, 1.56947f);
        scene.lights.push_back(&pointLightB);

        // === Camera ===
        auto camera = std::make_unique<Camera>(fow, ratio, 0.1f, 150.0f);
        camera->position = {0, 5, 15};
        camera->rotation = 0.f;
        camera->tilt     = -10.f;
        scene.camera = std::move(camera);

        // === Skybox ===
        if (!skybox) {
            skybox = new Skybox({
                "textures/skybox/right.bmp",
                "textures/skybox/left.bmp",
                "textures/skybox/top.bmp",
                "textures/skybox/bottom.bmp",
                "textures/skybox/front.bmp",
                "textures/skybox/back.bmp"
            });
        }

        // === Ground ===
        {
            auto ground = std::make_unique<Plane>(nullptr);
            ground->position = {0.0f, -1.0f, 0.0f};
            ground->scale    = {100.0f, 100.0f, 100.0f};
            scene.rootObjects.push_back(std::move(ground));
        }

        // === Scene objects ===


// ...
    // === Scene objects ===

        fs::path collectionDir = "D:/ppgso-public/data/Collection"; // or adjust to "D:/ppgso-public/data/Collection"
        fs::path texDir = "D:/ppgso-public/data/tex";

        // helpers

// Заменить существующий lambda findTextureFor на этот в `src/playground/SceneWindow.hpp`
auto findTextureFor = [&](const std::string &baseName)->std::pair<std::string,bool> {
    auto toLower = [](const std::string &s){
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);
        return out;
    };

    std::string baseLower = toLower(baseName);
    bool transparent = baseLower.find("glass") != std::string::npos;

    if (!fs::exists(texDir)) return {std::string(), transparent};

    // Разбиваем имя на токены по любым не-алфанумерным символам
    std::vector<std::string> tokens;
    std::string cur;
    for (char ch : baseLower) {
        if (std::isalnum((unsigned char)ch)) cur.push_back(ch);
        else {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        }
    }
    if (!cur.empty()) tokens.push_back(cur);

    // дополнительно добавить полное имя как токен
    if (!baseLower.empty()) tokens.insert(tokens.begin(), baseLower);

    int bestScore = 0;
    fs::path bestPath;

    for (auto &p : fs::directory_iterator(texDir)) {
        if (!p.is_regular_file()) continue;
        std::string stem = toLower(p.path().stem().string());      // имя без расширения
        std::string fname = toLower(p.path().filename().string()); // полное имя файла

        for (const auto &t : tokens) {
            if (stem == t
                || stem == t + "_base"
                || stem == t + "_basecolor"
                || stem == t + "basecolor"
                || stem == t + "_albedo"
                || stem == t + "albedo") {
                bestPath = p.path();
                bestScore = 100;
                break;
            }
            if (stem.find(t + "_base") != std::string::npos
                || stem.find(t + "base") != std::string::npos
                || stem.find(t + "_albedo") != std::string::npos) {
                if (bestScore < 90) { bestScore = 90; bestPath = p.path(); }
            }
            else if (stem.find(t) != std::string::npos) {
                if (bestScore < 50) { bestScore = 50; bestPath = p.path(); }
            }
            else if (fname.find(t) != std::string::npos) {
                if (bestScore < 10) { bestScore = 10; bestPath = p.path(); }
            }
        }
        if (bestScore == 100) break;
    }

    if (bestScore > 0) return { bestPath.string(), transparent };

    // FALLBACK: если ничего не найдено, использовать глобальную текстуру
    fs::path fallback = texDir / "default_baseColor.bmp"; // <- поместите здесь запасную текстуру
    if (fs::exists(fallback)) return { fallback.string(), transparent };

    return { std::string(), transparent };
};


        // карта групп по пути (относительный к collectionDir)
        std::unordered_map<std::string, Group*> groupMap;
        // Помещаем корневую группу
        groupMap["."] = nullptr;

        if (fs::exists(collectionDir)) {
            // Сначала создаём группы для всех директорий
            for (auto &entry : fs::recursive_directory_iterator(collectionDir)) {
                if (entry.is_directory()) {
                    std::string rel = std::filesystem::relative(entry.path(), collectionDir).string();
                    Group* parentG = nullptr;
                    auto parentPath = entry.path().parent_path();
                    if (parentPath != collectionDir && fs::exists(parentPath)) {
                        std::string relParent = std::filesystem::relative(parentPath, collectionDir).string();
                        if (groupMap.count(relParent)) parentG = groupMap[relParent];
                    }
                    // создаём группу и добавляем в сцену/родителя
                    auto gptr = std::make_unique<Group>(parentG);
                    // позиционируем по глубине для наглядности
                    int depth = std::distance(collectionDir.begin(), entry.path().begin()) - std::distance(collectionDir.begin(), collectionDir.begin());
                    gptr->position = glm::vec3(0.f, 0.0f, 0.0f);
                    Group* raw = gptr.get();
                    if (parentG) {
                        // добавляем как дочерний объект (через rootObjects — объект будет иметь parent pointer)
                        scene.rootObjects.push_back(std::move(gptr)); // если нужна полная иерархия должна использоваться специализированная система; упрощённо держим в root
                    } else {
                        scene.rootObjects.push_back(std::move(gptr));
                    }
                    groupMap[rel] = raw;
                }
            }

            // Теперь перебираем файлы .obj и создаём GenericModel
            std::unordered_map<std::string,int> folderIndex;
            for (auto &entry : fs::recursive_directory_iterator(collectionDir)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() != ".obj" && entry.path().extension() != ".OBJ") continue;

                auto parent = entry.path().parent_path();
                std::string relParent = ".";
                if (parent != collectionDir) relParent = std::filesystem::relative(parent, collectionDir).string();

                Group* parentGroup = nullptr;
                if (groupMap.count(relParent)) parentGroup = groupMap[relParent];

                std::string base = entry.path().stem().string();
                auto [tex, transparent] = findTextureFor(base);

                auto modelPtr = std::make_unique<GenericModel>(parentGroup, entry.path().string(), tex);
                // Размещаем объекты в сетке внутри папки
                int idx = folderIndex[relParent]++;
                int perRow = 6;
                float spacing = 4.0f;
                if (transparent) {
                    modelPtr->transparent = true;
                }
                modelPtr->position = glm::vec3(0.0f, 0.0f, 0.f);
                modelPtr->scale = {1.0f, 1.0f, 1.0f};
                // Если есть parentGroup — всё равно добавляем в rootObjects, parentObject уже установлен
                scene.rootObjects.push_back(std::move(modelPtr));
            }
        } else {
            // fallback: если папки нет, добавляем существующие building/balcony (как раньше)
            auto building = std::make_unique<Building>(nullptr);
            building->position = {1, 1, 1};
            scene.rootObjects.push_back(std::move(building));

            auto balcony = std::make_unique<Balcony>(nullptr);
            balcony->position = {1, 1, 1};
            scene.rootObjects.push_back(std::move(balcony));
        }

        {
            auto animatedPot = std::make_unique<GenericModel>(nullptr, "objects/pot.obj", "textures/pot.bmp");
            animatedPot->scale = {0.6f, 0.6f, 0.6f};
            animatedPot->keyframes.push_back(Keyframe(0.1f, {-5.f, 0.0f, -5.f}));
            animatedPot->keyframes.push_back(Keyframe(3.f, {-5.f, 0.0f, 5.f}, {0, 0, 0}, true, true));
            animatedPot->keyframes.push_back(Keyframe(3.f, {5.f, 0.0f, 5.f}, {0, 0, 0}, true, true));
            animatedPot->keyframes.push_back(Keyframe(3.f, {5.f, 0.0f, -5.f}, {0, 0, 0}, true, true));
            animatedPot->keyframes.push_back(Keyframe(3.f, {-5.f, 0.0f, -5.f}, {0, 0, 0}, true, true));
            scene.rootObjects.push_back(std::move(animatedPot));
        }
    }






    void handleInput(float dt) {
        if (!scene.camera) return;

        float mul = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) mul *= 2.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)  == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) mul *= 0.25f;

        const float camMove = camMoveSpeed  * mul * dt;

        const float yawDeg = scene.camera->rotation;
        const float yawRad = glm::radians(yawDeg);
        const glm::vec3 forward = glm::normalize(glm::vec3{ std::sin(yawRad), 0.0f, -std::cos(yawRad) });
        const glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3{0, 1, 0}));

        // WASD movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) scene.camera->position += forward * camMove;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) scene.camera->position -= forward * camMove;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) scene.camera->position -= right   * camMove;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) scene.camera->position += right   * camMove;

        // Vertical movement (R/F)
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) scene.camera->position.y += camMove;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) scene.camera->position.y -= camMove;

        // Camera rotation (Q/E)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) scene.camera->rotation += camRotSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) scene.camera->rotation -= camRotSpeed * dt;

        // Lamp movement (I/K/J/L/U/O)

    }

    void onResize(int width, int height) {
        size_x = width;
        size_y = height;
        if (height <= 0) height = 1;
        ratio = static_cast<float>(width) / static_cast<float>(height);

        glViewport(0, 0, width, height);

        if (scene.camera) {
            float fovRad = (ppgso::PI / 180.0f) * fow;
            scene.camera->projectionMatrix = glm::perspective(fovRad, ratio, 0.1f, 150.0f);
        }
    }

public:
    SceneWindow(std::string WINDOW_NAME, int SIZE_X, int SIZE_Y)
        : Window{std::move(WINDOW_NAME), SIZE_X, SIZE_Y} {

        ratio = float(SIZE_X) / float(SIZE_Y);
        size_x = SIZE_X;
        size_y = SIZE_Y;

        std::random_device rd;
        rng = std::mt19937{rd()};

        glfwSetInputMode(window, GLFW_STICKY_KEYS, 0);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);

        createShadowResources();
        createPointShadowResources();
        // Компилируем общий shadow-шейдер один раз
        shadowShader = std::make_unique<ppgso::Shader>(shadow_vert_glsl, shadow_frag_glsl);

        initScene();

        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window,
            [](GLFWwindow* win, int w, int h) {
                auto self = static_cast<SceneWindow*>(glfwGetWindowUserPointer(win));
                if (self) self->onResize(w, h);
            });

        glfwSetCursorPosCallback(window,
            [](GLFWwindow* win, double xpos, double ypos) {
                auto self = static_cast<SceneWindow*>(glfwGetWindowUserPointer(win));
                if (!self || !self->scene.camera) return;

                if (self->firstMouse) {
                    self->lastX = xpos;
                    self->lastY = ypos;
                    self->firstMouse = false;
                }

                double xoffset = xpos - self->lastX;
                double yoffset = self->lastY - ypos;

                self->lastX = xpos;
                self->lastY = ypos;

                xoffset *= self->mouseSensitivity;
                yoffset *= self->mouseSensitivity;

                self->scene.camera->rotation += static_cast<float>(xoffset);
                self->scene.camera->tilt += static_cast<float>(yoffset);

                if (self->scene.camera->tilt > 89.0f)  self->scene.camera->tilt = 89.0f;
                if (self->scene.camera->tilt < -89.0f) self->scene.camera->tilt = -89.0f;
            });

        onResize(SIZE_X, SIZE_Y);
    }

    void onIdle() override {
        if (loadTime == -1.f) loadTime = (float) glfwGetTime();

        static float sceneTime = -1.f;
        if (sceneTime == -1.f)
            sceneTime = (float) glfwGetTime() - loadTime;

        float dt = animate ? (float) glfwGetTime() - loadTime - sceneTime : 0.f;
        sceneTime = (float) glfwGetTime() - loadTime;

        handleInput(dt);
        scene.update(dt);


        // Build list of shadow-casting lights and compute their light space matrices
        // Light indices must match the order in scene.lights
        std::vector<std::pair<Light*, int>> shadowCasters2D; // pair<light, lightIndex>
        std::vector<std::pair<Light*, int>> pointShadowCasters;

        // Check mainlight first (index 0 in scene.lights)
        if (scene.mainlight) {
            shadowCasters2D.push_back({scene.mainlight.get(), 0});
            scene.lightViewMatrix       = scene.mainlight->getLightView();
            scene.lightProjectionMatrix = scene.mainlight->lightProjectionMatrix;
        }

        // Add remaining lights (indices 1, 2, 3 in scene.lights)
        // pointLightA = index 1, spotLight = index 2, pointLightB = index 3
        int lightIndex = 1;
        for (Light* light : {&pointLightA, &spotLight, &pointLightB}) {

            if (light->type == LightType::Point) {
                if (pointShadowCasters.size() < static_cast<size_t>(NUM_POINT_SHADOW_MAPS)) {
                    pointShadowCasters.push_back({light, lightIndex});
                }
            } else {
                if (shadowCasters2D.size() < static_cast<size_t>(NUM_SHADOW_MAPS)) {
                    shadowCasters2D.push_back({light, lightIndex});
                }
            }
            lightIndex++;
        }

        // Update scene with shadow caster info
        scene.numShadowMaps = static_cast<int>(shadowCasters2D.size());
        scene.numPointShadowMaps = static_cast<int>(pointShadowCasters.size());
        for (int i = 0; i < NUM_SHADOW_MAPS; ++i) {
            if (i < static_cast<int>(shadowCasters2D.size())) {
                Light* light = shadowCasters2D[i].first;
                scene.shadowCasterIndices[i] = shadowCasters2D[i].second;

                // Compute light space matrix based on light type
                if (light->type == LightType::Directional) {
                    scene.lightSpaceMatrices[i] = computeDirectionalLightSpaceMatrix(*light);
                } else {
                    // Spotlight - use perspective projection
                    float near = 0.1f;
                    float far = light->maxDist > 0 ? light->maxDist : 100.0f;
                    glm::mat4 proj = glm::perspective(glm::radians(light->outerCutOff * 2.0f), 1.0f, near, far);
                    glm::vec3 target = light->position + light->effectiveDirection();
                    glm::mat4 view = glm::lookAt(light->position, target, glm::vec3(0.0f, 1.0f, 0.0f));
                    scene.lightSpaceMatrices[i] = proj * view;
                }
            } else {
                scene.shadowCasterIndices[i] = -1;
                scene.lightSpaceMatrices[i] = glm::mat4(1.0f);
            }
        }
        for (int i = 0; i < NUM_POINT_SHADOW_MAPS; ++i) {
            if (i < static_cast<int>(pointShadowCasters.size())) {
                Light* light = pointShadowCasters[i].first;
                scene.pointShadowCasterIndices[i] = pointShadowCasters[i].second;
                scene.pointShadowFarPlane[i] = light->maxDist > 0 ? light->maxDist : 100.0f;
            } else {
                scene.pointShadowCasterIndices[i] = -1;
                scene.pointShadowFarPlane[i] = 0.0f;
            }
        }

        // PASS 1: Shadow map rendering for each shadow-casting light
        glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
        glDisable(GL_CULL_FACE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        for (int i = 0; i < scene.numShadowMaps && i < NUM_SHADOW_MAPS; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBOs[i]);
            glClear(GL_DEPTH_BUFFER_BIT);

            if (shadowShader) {
                shadowShader->use();
                shadowShader->setUniform("lightSpaceMatrix", scene.lightSpaceMatrices[i]);
                shadowShader->setUniform("isPointLight", false);
                scene.renderForShadow(0);
            }
        }

        // PASS 1b: Point light shadow cubemaps
        glViewport(0, 0, POINT_SHADOW_SIZE, POINT_SHADOW_SIZE);
        for (int i = 0; i < scene.numPointShadowMaps && i < NUM_POINT_SHADOW_MAPS; ++i) {
            Light* light = pointShadowCasters[i].first;
            float farPlane = scene.pointShadowFarPlane[i];
            float nearPlane = 0.1f;
            auto shadowTransforms = buildPointShadowTransforms(light->position, nearPlane, farPlane);

            for (int face = 0; face < 6; ++face) {
                glBindFramebuffer(GL_FRAMEBUFFER, pointShadowMapFBOs[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, pointShadowMaps[i], 0);
                glClear(GL_DEPTH_BUFFER_BIT);

                if (shadowShader) {
                    shadowShader->use();
                    shadowShader->setUniform("lightSpaceMatrix", shadowTransforms[face]);
                    shadowShader->setUniform("lightPos", light->position);
                    shadowShader->setUniform("far_plane", farPlane);
                    shadowShader->setUniform("isPointLight", true);
                    scene.renderForShadow(0);
                }
            }
        }

        glUseProgram(0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // PASS 2: Main scene rendering
        glViewport(0, 0, size_x, size_y);
        glClearColor(.2f, .2f, .3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (scene.camera && skybox) {
            glDepthMask(GL_FALSE);
            skybox->render(scene.camera->viewMatrix, scene.camera->projectionMatrix);
            glDepthMask(GL_TRUE);
        }

        // Bind all shadow maps to texture units
        for (int i = 0; i < NUM_SHADOW_MAPS; ++i) {
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_2D, shadowMaps[i]);
        }
        for (int i = 0; i < NUM_POINT_SHADOW_MAPS; ++i) {
            glActiveTexture(GL_TEXTURE5 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowMaps[i]);
        }
        std::cout<<scene.camera->position.x << " " << scene.camera->position.y << " " << scene.camera->position.z <<std::endl;
        scene.render(shadowMaps, scene.numShadowMaps);

        // Unbind shadow maps
        for (int i = 0; i < NUM_SHADOW_MAPS; ++i) {
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        for (int i = 0; i < NUM_POINT_SHADOW_MAPS; ++i) {
            glActiveTexture(GL_TEXTURE5 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }
        glActiveTexture(GL_TEXTURE0);
    }

    void onKey(int key, int, int action, int) override {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }


    }

};

#endif //PPGSO_SCENEWINDOW_HPP
