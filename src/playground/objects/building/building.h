//
// Created by Pavel on 25.11.2025.
//

#ifndef PPGSO_BUILDING_H
#define PPGSO_BUILDING_H



#include <memory>
#include <ppgso/ppgso.h>
#include "src/playground/scene.h"
#include "src/playground/object.h"

class Building : public Object {
private:
    static std::unique_ptr<ppgso::Mesh> mesh;
    static std::unique_ptr<ppgso::Shader> shader;         // Phong
    static std::unique_ptr<ppgso::Texture> texture;

public:
    Building(Object* parent);

    void checkCollisions(Scene &scene, float dt)  {};

    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) ;

    void render(Scene &scene, GLuint depthMap) ;

    void renderForShadow(Scene &scene) ;
    void renderForShadow(Scene &scene, GLuint shadowProgram) ;
};

#endif //PPGSO_BUILDING_H