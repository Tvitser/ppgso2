#include "puzzle.h"

#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>
#include <cfloat>

std::unique_ptr<ppgso::Mesh> Puzzle::mesh;
std::unique_ptr<ppgso::Shader> Puzzle::shader;
std::unique_ptr<ppgso::Texture> Puzzle::texture;

Puzzle::Puzzle(Object* parent, const glm::vec3& initialPosition, const glm::vec3& initialRotation) {
    parentObject = parent;
    position = initialPosition;
    rotation = initialRotation;
    scale = {0.5f, 0.5f, 0.5f}; // Make puzzle a reasonable size
    radius = 0.5f;
    weight = mass;

    if (!mesh) {
        // Use sphere for puzzle piece
        mesh = std::make_unique<ppgso::Mesh>("objects/sphere.obj");
    }
    if (!shader) {
        shader = std::make_unique<ppgso::Shader>(phong_vert_glsl, phong_frag_glsl);
    }
    if (!texture) {
        texture = std::make_unique<ppgso::Texture>(ppgso::image::loadBMP("textures/ground.bmp"));
    }
}

glm::vec3 Puzzle::getMin() const {
    // Get the 8 corners of the unit cube and transform them by model matrix
    glm::vec3 corners[8] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}
    };

    glm::vec3 minBound(FLT_MAX);
    for (const auto& corner : corners) {
        glm::vec4 transformed = modelMatrix * glm::vec4(corner, 1.0f);
        minBound = glm::min(minBound, glm::vec3(transformed));
    }
    return minBound;
}

glm::vec3 Puzzle::getMax() const {
    glm::vec3 corners[8] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}
    };

    glm::vec3 maxBound(-FLT_MAX);
    for (const auto& corner : corners) {
        glm::vec4 transformed = modelMatrix * glm::vec4(corner, 1.0f);
        maxBound = glm::max(maxBound, glm::vec3(transformed));
    }
    return maxBound;
}

bool Puzzle::checkCollisionWithObject(Object* other) const {
    if (!other || other == this) return false;

    // Get bounding boxes
    glm::vec3 myMin = getMin();
    glm::vec3 myMax = getMax();

    // For other objects, calculate bounds from their model matrix
    glm::vec3 otherCorners[8] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}
    };

    glm::vec3 otherMin(FLT_MAX);
    glm::vec3 otherMax(-FLT_MAX);
    for (const auto& corner : otherCorners) {
        glm::vec4 transformed = other->modelMatrix * glm::vec4(corner, 1.0f);
        otherMin = glm::min(otherMin, glm::vec3(transformed));
        otherMax = glm::max(otherMax, glm::vec3(transformed));
    }

    // AABB collision detection
    return (myMin.x <= otherMax.x && myMax.x >= otherMin.x) &&
           (myMin.y <= otherMax.y && myMax.y >= otherMin.y) &&
           (myMin.z <= otherMax.z && myMax.z >= otherMin.z);
}

void Puzzle::checkCollisions(Scene &scene, float dt) {
    isGrounded = false;

    glm::vec3 myMin = getMin();
    glm::vec3 myMax = getMax();

    // Check collision with ground (y = -1.0f based on plane position in scene)
    float groundLevel = -1.0f;
    if (myMin.y <= groundLevel) {
        // Collision with ground
        float penetration = groundLevel - myMin.y;
        position.y += penetration;
        
        if (velocity.y < 0) {
            velocity.y = -velocity.y * restitution;
            if (glm::abs(velocity.y) < 0.1f) {
                velocity.y = 0.0f;
                isGrounded = true;
            }
        }
        
        // Apply friction when grounded
        if (isGrounded) {
            velocity.x *= 0.95f;
            velocity.z *= 0.95f;
            angularVelocity *= 0.95f;
        }
    }

    // Check collisions with other objects in the scene
    for (auto& obj : scene.rootObjects) {
        if (obj.get() == this) continue;

        if (checkCollisionWithObject(obj.get())) {
            // Simple collision response: stop vertical movement
            if (velocity.y < 0) {
                velocity.y = -velocity.y * restitution;
                if (glm::abs(velocity.y) < 0.1f) {
                    velocity.y = 0.0f;
                    isGrounded = true;
                }
            }
            
            // Move puzzle slightly up to avoid penetration
            glm::vec3 otherMin(FLT_MAX);
            glm::vec3 otherMax(-FLT_MAX);
            glm::vec3 otherCorners[8] = {
                {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
                {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f},
                {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f},
                {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}
            };
            for (const auto& corner : otherCorners) {
                glm::vec4 transformed = obj->modelMatrix * glm::vec4(corner, 1.0f);
                otherMin = glm::min(otherMin, glm::vec3(transformed));
                otherMax = glm::max(otherMax, glm::vec3(transformed));
            }
            
            float penetrationDepth = otherMax.y - myMin.y;
            if (penetrationDepth > 0 && penetrationDepth < 1.0f) {
                position.y += penetrationDepth;
            }
        }
    }
}

bool Puzzle::update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3 parentRotation) {
    // Initialize velocity from speed on first update
    if (firstUpdate) {
        velocity = speed;
        firstUpdate = false;
    }
    
    // Apply gravity
    if (!isGrounded) {
        velocity.y += gravity * dt;
    }

    // Update position based on velocity
    position += velocity * dt;

    // Update rotation based on angular velocity
    rotation += angularVelocity * dt;

    // Generate model matrix
    generateModelMatrix(parentModelMatrix);

    return true;
}

void Puzzle::render(Scene &scene, GLuint depthMap) {
    shader->use();

    // Use correct uniform names matching phong shader
    shader->setUniform("projection", scene.camera->projectionMatrix);
    shader->setUniform("view", scene.camera->viewMatrix);
    shader->setUniform("model", modelMatrix);

    // Set multiple light space matrices
    shader->setUniform("numShadowMaps", scene.numShadowMaps);
    for (int i = 0; i < MAX_SHADOW_MAPS; ++i) {
        std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
        shader->setUniform(uniformName, scene.lightSpaceMatrices[i]);
    }

    shader->setUniform("Texture", *texture);

    // Shadow maps are already bound by SceneWindow to texture units 1-4
    shader->setUniform("shadowMap0", 1);
    shader->setUniform("shadowMap1", 2);
    shader->setUniform("shadowMap2", 3);
    shader->setUniform("shadowMap3", 4);

    scene.renderLight(shader, true);

    mesh->render();
}

void Puzzle::renderForShadow(Scene &scene) {
    generateModelMatrix(parentObject ? parentObject->modelMatrix : glm::mat4{1.0f});

    // Use the currently bound shadow shader from SceneWindow
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint locModel = glGetUniformLocation(static_cast<GLuint>(currentProgram), "ModelMatrix");
    if (locModel >= 0) {
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    }

    mesh->render();
}
