#version 330
// The inputs will be fed by the vertex buffer objects
layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

// Matrices as program attributes
uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;
uniform bool useInstancing;
uniform float instanceRadius;
uniform int instanceSeed;

const float UINT_MAX_FLOAT = 4294967295.0;
const float TWO_PI = 6.28318530718;
const uint LCG_A = 1664525u;
const uint LCG_C = 1013904223u;

// This will be passed to the fragment shader
out vec2 texCoord;

uint hash(uint x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

float rand01(uint x) {
  return float(hash(x)) / UINT_MAX_FLOAT;
}

vec2 randomOffset(uint id) {
  uint base = id ^ uint(instanceSeed);
  float r = sqrt(rand01(base)) * instanceRadius;
  float theta = rand01(base * LCG_A + LCG_C) * TWO_PI;
  return vec2(cos(theta), sin(theta)) * r;
}

void main() {
  // Copy the input to the fragment shader
  texCoord = TexCoord;

  mat4 instanceModel = ModelMatrix;
  if (useInstancing) {
    vec2 offset = randomOffset(uint(gl_InstanceID));
    mat4 translateMat = mat4(1.0);
    translateMat[3] = vec4(offset.x, 0.0, offset.y, 1.0);
    instanceModel = translateMat * ModelMatrix;
  }

  // Calculate the final position on screen
  gl_Position = ProjectionMatrix * ViewMatrix * instanceModel * vec4(Position, 1.0);
}
