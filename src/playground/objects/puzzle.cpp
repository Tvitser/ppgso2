#include "puzzle.h"
#include "src/playground/scene.h"

#include <shader/phong_vert_glsl.h>
#include <shader/phong_frag_glsl.h>
#include <glm/gtc/type_ptr.hpp>

std::unique_ptr<ppgso::Mesh> Puzzle::mesh;
std::unique_ptr<ppgso::Shader> Puzzle::shader;
std::unique_ptr<ppgso::Texture> Puzzle::texture;

Puzzle::Puzzle(Object* parent) {
    parentObject = parent;
    position = {0, 0, 0};
    rotation = {0, 0, 0};
    scale = {1, 1, 1};
    velocity = {0, 0, 0};

    if (!mesh) {
        mesh = std::make_unique<ppgso::Mesh>("objects/cube_simple.obj");
    }
    if (!shader) {
        shader = std::make_unique<ppgso::Shader>(phong_vert_glsl, phong_frag_glsl);
    }
    if (!texture) {
        texture = std::make_unique<ppgso::Texture>(ppgso::image::loadBMP("textures/brick.bmp"));
    }
}

void Puzzle::checkCollisions(Scene &scene, float dt) {
    // Reset ground flag at start of collision check
    isOnGround = false;
    
    // Check collision with ground (plane at y = -1)
    const float groundY = -1.0f;
    const float puzzleHalfHeight = scale.y * 0.5f;
    const float puzzleBottomY = position.y - puzzleHalfHeight;
    
    if (puzzleBottomY <= groundY) {
        // Collision with ground
        position.y = groundY + puzzleHalfHeight;
        velocity.y = 0;
        isOnGround = true;
    }
    
    // Extract puzzle's AABB from its model matrix
    glm::vec3 puzzlePos = glm::vec3(modelMatrix[3]);
    glm::vec3 puzzleScale = scale; // Use puzzle's scale
    glm::vec3 puzzleMin = puzzlePos - puzzleScale * 0.5f;
    glm::vec3 puzzleMax = puzzlePos + puzzleScale * 0.5f;
    
    // Check collision with other objects in the scene using AABB
    for (auto& obj : scene.rootObjects) {
        if (obj.get() == this) continue; // Skip self
        
        // Extract object's position and scale from its model matrix
        glm::vec3 objPos = glm::vec3(obj->modelMatrix[3]);
        glm::vec3 objScale = obj->scale;
        
        // Skip objects with zero scale (invalid or uninitialized)
        if (objScale.x <= 0.001f || objScale.y <= 0.001f || objScale.z <= 0.001f) {
            continue;
        }
        
        // Calculate object's AABB
        glm::vec3 objMin = objPos - objScale * 0.5f;
        glm::vec3 objMax = objPos + objScale * 0.5f;
        
        // AABB collision test
        bool collisionX = puzzleMax.x >= objMin.x && puzzleMin.x <= objMax.x;
        bool collisionY = puzzleMax.y >= objMin.y && puzzleMin.y <= objMax.y;
        bool collisionZ = puzzleMax.z >= objMin.z && puzzleMin.z <= objMax.z;
        
        if (collisionX && collisionY && collisionZ) {
            // Collision detected - calculate overlap on each axis
            float overlapX = std::min(puzzleMax.x - objMin.x, objMax.x - puzzleMin.x);
            float overlapY = std::min(puzzleMax.y - objMin.y, objMax.y - puzzleMin.y);
            float overlapZ = std::min(puzzleMax.z - objMin.z, objMax.z - puzzleMin.z);
            
            // Resolve collision along the axis with minimum overlap
            if (overlapY <= overlapX && overlapY <= overlapZ && velocity.y < 0) {
                // Resolve vertical collision (puzzle falling onto object)
                float separation = overlapY;
                position.y += separation;
                velocity.y = 0;
                isOnGround = true;
            } else if (overlapX <= overlapY && overlapX <= overlapZ) {
                // Resolve horizontal X collision
                float separation = overlapX;
                if (puzzlePos.x < objPos.x) {
                    position.x -= separation;
                } else {
                    position.x += separation;
                }
            } else {
                // Resolve horizontal Z collision
                float separation = overlapZ;
                if (puzzlePos.z < objPos.z) {
                    position.z -= separation;
                } else {
                    position.z += separation;
                }
            }
        }
    }
}

bool Puzzle::update(Scene &scene, float dt, glm::mat4 parentModelMatrix, glm::vec3) {
    // Apply gravity if not on ground
    if (!isOnGround) {
        velocity.y += gravity * dt;
    }
    
    // Update position based on velocity
    position += velocity * dt;
    
    // Add slight rotation for visual effect
    rotation.y += dt * 0.5f;
    rotation.x += dt * 0.3f;
    
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
    shader->setUniform("pointShadowMaps[0]", 5);
    shader->setUniform("pointShadowMaps[1]", 6);

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
