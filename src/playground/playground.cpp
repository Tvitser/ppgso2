#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include "SceneWindow.hpp"

const int SIZE_X = 1920;
const int SIZE_Y = 1080;

int main() {
  // Create our window
  auto window = SceneWindow{"Test Scene", SIZE_X, SIZE_Y};

  // Main execution loop
  while (window.pollEvents()) {}

  return EXIT_SUCCESS;
}
