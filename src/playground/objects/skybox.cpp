#include "skybox.h"
#include <shader/skybox_vert_glsl.h>
#include <shader/skybox_frag_glsl.h>

Skybox::Skybox(const std::vector<std::string>& faces)
  : shader{skybox_vert_glsl, skybox_frag_glsl}, textureID(0), VAO(0), VBO(0) {

  // Создаём кубическую текстуру
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  // Загружаем все 6 граней кубмапы
  for (unsigned int i = 0; i < faces.size(); i++) {
    auto image = ppgso::image::loadBMP(faces[i]);
    auto& framebuffer = image.getFramebuffer();

    glTexImage2D(
      GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
      0,
      GL_RGB,
      image.width,
      image.height,
      0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      framebuffer.data()
    );
  }

  // Параметры текстуры
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // VAO/VBO для куба скайбокса
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

  glBindVertexArray(0);
}

void Skybox::render(const glm::mat4& view, const glm::mat4& projection) {
  glDepthFunc(GL_LEQUAL);
  shader.use();

  glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
  shader.setUniform("ViewMatrix", viewNoTranslation);
  shader.setUniform("ProjectionMatrix", projection);
  shader.setUniform("skybox", 0); // <--- ВАЖНО

  glBindVertexArray(VAO);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  glDepthFunc(GL_LESS);
}


Skybox::~Skybox() {
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteTextures(1, &textureID);
}
