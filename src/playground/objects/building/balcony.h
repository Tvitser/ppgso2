//
// Created by Pavel on 29.11.2025.
//

#ifndef PPGSO_BALCONY_H
#define PPGSO_BALCONY_H


//
// Created by Pavel on 25.11.2025.
//





#include <memory>
#include <ppgso/ppgso.h>
#include "src/playground/scene.h"
#include "src/playground/object.h"

class Balcony : public Object {
private:
    static std::unique_ptr<ppgso::Mesh> mesh;
    static std::unique_ptr<ppgso::Shader> shader;         // Phong
    static std::unique_ptr<ppgso::Texture> texture;

public:
    Balcony(Object* parent);

    void checkCollisions(Scene &scene, float dt)  {};

    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) ;

    void render(Scene &scene, GLuint depthMap) ;

    void renderForShadow(Scene &scene) ;
    void renderForShadow(Scene &scene, GLuint shadowProgram) ;
};


#endif //PPGSO_BALCONY_H