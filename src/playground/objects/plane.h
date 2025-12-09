#ifndef PPGSO_PLANE_H
#define PPGSO_PLANE_H

#include <memory>
#include <ppgso/ppgso.h>
#include "src/playground/scene.h"
#include "src/playground/object.h"

class Plane : public Object {
private:
    static std::unique_ptr<ppgso::Mesh> mesh;
    static std::unique_ptr<ppgso::Shader> shader;
    static std::unique_ptr<ppgso::Texture> texture;

public:
    Plane(Object* parent);

    void checkCollisions(Scene &scene, float dt) override {};

    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) override;

    void render(Scene &scene, GLuint depthMap) override;

    void renderForShadow(Scene &scene) override;
};

#endif //PPGSO_PLANE_H
