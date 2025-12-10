#ifndef PPGSO_PUZZLE_H
#define PPGSO_PUZZLE_H

#include <memory>
#include <ppgso/ppgso.h>
#include "src/playground/scene.h"
#include "src/playground/object.h"

class Puzzle : public Object {
private:
    static std::unique_ptr<ppgso::Mesh> mesh;
    static std::unique_ptr<ppgso::Shader> shader;
    static std::unique_ptr<ppgso::Texture> texture;

    // Physics properties
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    glm::vec3 angularVelocity{0.0f, 0.0f, 0.0f};
    const float gravity = -9.81f;
    const float mass = 1.0f;
    const float restitution = 0.3f; // Bounciness coefficient
    bool isGrounded = false;

    // Collision bounds based on model matrix
    glm::vec3 getMin() const;
    glm::vec3 getMax() const;
    bool checkCollisionWithObject(Object* other) const;

public:
    Puzzle(Object* parent, const glm::vec3& initialPosition, const glm::vec3& initialRotation);

    void checkCollisions(Scene &scene, float dt) override;

    bool update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) override;

    void render(Scene &scene, GLuint depthMap) override;

    void renderForShadow(Scene &scene) override;
};

#endif //PPGSO_PUZZLE_H
