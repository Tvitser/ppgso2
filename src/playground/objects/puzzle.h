#ifndef PPGSO_PUZZLE_H
#define PPGSO_PUZZLE_H

#include <memory>
#include <ppgso/ppgso.h>
#include "src/playground/object.h"

// Forward declaration
class Scene;

class Puzzle : public Object {
private:
    static std::unique_ptr<ppgso::Mesh> mesh;
    static std::unique_ptr<ppgso::Shader> shader;
    static std::unique_ptr<ppgso::Texture> texture;

    // Physics properties
    glm::vec3 velocity{0, 0, 0};
    const float gravity = -9.8f;
    const float collisionRadius = 0.5f;
    bool isOnGround = false;

public:
    Puzzle(Object* parent);

    void checkCollisions(Scene &scene, float dt) override;

    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) override;

    void render(Scene &scene, GLuint depthMap) override;

    void renderForShadow(Scene &scene) override;
};

#endif //PPGSO_PUZZLE_H
