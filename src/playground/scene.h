#ifndef _PPGSO_SCENE_H
#define _PPGSO_SCENE_H

#include <memory>
#include <map>
#include <list>
#include <vector>

#include "object.h"
#include "camera.h"
#include "light.h"
#include "mainlight.h"

constexpr int MAX_SHADOW_MAPS = 4;
constexpr int MAX_POINT_SHADOW_MAPS = 2;

class Scene {
public:
 void update(float time);
 void render(GLuint depthMaps[MAX_SHADOW_MAPS], int numShadowMaps);
 void renderForShadow(GLuint depthMap);
 void close();

 void renderLight(std::unique_ptr<ppgso::Shader> &shader, bool onlyMain = true);
 std::unique_ptr<Camera> camera;
 std::list< std::unique_ptr<Object> > rootObjects;

 std::list<Light *> lights = {};
 std::unique_ptr<MainLight> mainlight = nullptr;

 bool showBoundingBoxes = false;
 bool showFPS = false;
 float lastFPSOutputTime = 0.f;
 short scene_id = 0;

 // Support for multiple shadow-casting lights
 int numShadowMaps = 0;
 glm::mat4 lightSpaceMatrices[MAX_SHADOW_MAPS];
 int shadowCasterIndices[MAX_SHADOW_MAPS]; // Maps shadow map index to light index
 int numPointShadowMaps = 0;
 int pointShadowCasterIndices[MAX_POINT_SHADOW_MAPS];
 float pointShadowFarPlane[MAX_POINT_SHADOW_MAPS];

 // Legacy single light (for backward compatibility)
 glm::mat4 lightProjectionMatrix{1.f};
 glm::mat4 lightViewMatrix{1.f};
};

#endif // _PPGSO_SCENE_H