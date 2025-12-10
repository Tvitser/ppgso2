# Puzzle Object Implementation

## Overview
This implementation adds a falling puzzle object that can be spawned at the camera's position using the 'P' key. The puzzle object features realistic physics including gravity and collision detection.

## Features

### 1. Physics System
- **Gravity**: The puzzle object experiences constant downward acceleration (-9.8 m/s²)
- **Velocity**: The object's velocity is updated each frame based on gravity
- **Position Update**: Position is updated based on velocity each frame

### 2. Collision Detection
The puzzle object implements two types of collision detection:

#### Ground Collision
- Detects collision with the ground plane (y = -1.0)
- Uses the puzzle's scale to determine its half-height
- When the bottom of the puzzle (position.y - scale.y * 0.5) touches or goes below the ground, it:
  - Repositions the puzzle to rest exactly on the ground
  - Sets vertical velocity to 0
  - Sets the `isOnGround` flag

#### Object-to-Object Collision
- Uses AABB (Axis-Aligned Bounding Box) collision detection
- Extracts position and scale from each object's model matrix
- Calculates bounding boxes: min = position - scale * 0.5, max = position + scale * 0.5
- When collision is detected:
  - Calculates overlap on each axis (X, Y, Z)
  - Resolves collision along the axis with minimum overlap
  - For vertical collisions (Y-axis), stops vertical movement and sets `isOnGround`
  - For horizontal collisions, pushes the puzzle away from the object

### 3. Visual Effects
- The puzzle rotates while falling for visual interest
- Uses brick texture for visibility
- Properly integrated with the scene's lighting and shadow system

## Files Modified/Created

### New Files
1. **src/playground/objects/puzzle.h** - Header file defining the Puzzle class
2. **src/playground/objects/puzzle.cpp** - Implementation of the Puzzle class
3. **data/objects/cube_simple.obj** - Simple cube mesh for the puzzle

### Modified Files
1. **src/playground/SceneWindow.hpp** - Added include for puzzle.h and key binding for 'P' key
2. **CMakeLists.txt** - Added puzzle.cpp and puzzle.h to the build

## Usage

### Spawning a Puzzle
1. Run the application
2. Navigate to your desired position using WASD keys
3. Press the **'P'** key to spawn a puzzle at the camera's current position
4. The puzzle will immediately begin falling due to gravity

### Expected Behavior
- The puzzle spawns at the camera position
- It falls downward due to gravity
- If it falls onto the ground or another object (like a table), it will stop and rest on that surface
- Multiple puzzles can be spawned and will interact with each other

## Technical Details

### Class Structure
```cpp
class Puzzle : public Object {
private:
    glm::vec3 velocity;           // Current velocity
    const float gravity = -9.8f;  // Gravity acceleration
    bool isOnGround;              // Whether the puzzle is resting on a surface
    
public:
    void checkCollisions(Scene &scene, float dt);  // Collision detection using AABB
    bool update(Scene &scene, float dt, ...);      // Physics update
    void render(Scene &scene, GLuint depthMap);    // Rendering
    void renderForShadow(Scene &scene);            // Shadow rendering
};
```

### Update Loop Flow
1. **checkCollisions()** - Called first to detect any collisions
   - Checks ground collision
   - Checks collision with all other objects in the scene
   
2. **update()** - Called to update physics
   - Applies gravity if not on ground
   - Updates position based on velocity
   - Resets ground flag (will be set again if still colliding)
   - Adds rotation for visual effect
   - Updates model matrix

3. **render()** - Called to draw the object
   - Configures shader uniforms
   - Renders the mesh with proper lighting and shadows

## Limitations and Future Improvements

### Current Limitations
1. Uses AABB collision detection (doesn't account for object rotation)
2. No bounce or friction simulation (objects stop immediately on collision)
3. No horizontal velocity (objects fall straight down)
4. Requires objects to have proper scale values set for collision detection

### Potential Improvements
1. Add more realistic physics (bounce, friction, angular momentum)
2. Implement OBB (Oriented Bounding Box) collision detection to account for rotations
3. Add different puzzle shapes and textures
4. Allow puzzles to have initial velocity from camera movement
5. Add sound effects for collision events
6. Implement puzzle stacking with proper stability
7. Support for convex mesh collision detection

## Building and Testing

### Build Requirements
- CMake 3.5+
- OpenGL
- GLFW3
- GLEW
- GLM
- Assimp (optional)

### Build Commands
```bash
mkdir -p _build
cd _build
cmake ..
make
```

### Testing
1. Run the application: `./playground`
2. Use WASD to move around
3. Position camera above ground or table
4. Press 'P' to spawn puzzle
5. Observe the puzzle falling and colliding with surfaces

## Integration with Existing Code

The implementation follows the existing code patterns:
- Uses the Object base class interface
- Implements required virtual methods (update, render, renderForShadow, checkCollisions)
- Uses static members for shared resources (mesh, shader, texture)
- Properly integrates with the scene's lighting and shadow system
- Follows the same rendering patterns as other objects (Plane, Building, etc.)
