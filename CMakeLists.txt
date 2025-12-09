# This CMake script is designed for CLion IDE and QTCreator projects on Windows
cmake_minimum_required(VERSION 3.5)
project(ppgso CXX)

#
# CONFIGURATION
#
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/_install)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Remove problematic compiler flags and suppress GLM warnings
if (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-sign-conversion -Wno-unused-parameter -Wno-pragmas -Wno-pedantic")
endif ()

#
# DEPENDENCIES
#
if (MINGW)
  set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "${CMAKE_SOURCE_DIR}/dependencies/include/")
  set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${CMAKE_SOURCE_DIR}/dependencies/lib/mingw")
elseif (MSVC)
  set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "${CMAKE_SOURCE_DIR}/dependencies/include/")
  if(MSVC_VERSION EQUAL 1500)
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${CMAKE_SOURCE_DIR}/dependencies/lib/vc2015")
  elseif(MSVC_VERSION GREATER_EQUAL 1900)
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${CMAKE_SOURCE_DIR}/dependencies/lib/vc2019")
  endif()
endif ()

find_package(OpenGL REQUIRED)

find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h PATHS ${CMAKE_INCLUDE_PATH})
find_library(GLFW_LIBRARY NAMES glfw3 glfw PATHS ${CMAKE_LIBRARY_PATH})

find_path(GLEW_INCLUDE_DIR NAMES GL/glew.h PATHS ${CMAKE_INCLUDE_PATH})
find_library(GLEW_LIBRARY NAMES GLEW glew32 PATHS ${CMAKE_LIBRARY_PATH})

find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp PATHS ${CMAKE_INCLUDE_PATH})

find_path(ASSIMP_INCLUDE_DIR NAMES assimp/ai_metadata.h PATHS ${CMAKE_INCLUDE_PATH})
find_library(ASSIMP_LIBRARY NAMES assimp PATHS ${CMAKE_LIBRARY_PATH})

if (GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
  set(GLFW_FOUND TRUE)
  set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIR})
  set(GLFW_LIBRARIES ${GLFW_LIBRARY})
endif()

if (GLEW_INCLUDE_DIR AND GLEW_LIBRARY)
  set(GLEW_FOUND TRUE)
  set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
  set(GLEW_LIBRARIES ${GLEW_LIBRARY})
endif()

if (GLM_INCLUDE_DIR)
  set(GLM_FOUND TRUE)
  set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
endif()

if (ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARY)
  set(ASSIMP_FOUND TRUE)
  set(ASSIMP_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIR})
  set(ASSIMP_LIBRARIES ${ASSIMP_LIBRARY})
  add_definitions(-DUSE_ASSIMP=1)
  message(STATUS "ASSIMP found")
else()
  message(STATUS "ASSIMP not found, using fallback")
endif()

if (NOT GLFW_FOUND)
  message(FATAL_ERROR "GLFW not found!")
endif()
if (NOT GLEW_FOUND)
  message(FATAL_ERROR "GLEW not found!")
endif()
if (NOT GLM_FOUND)
  message(FATAL_ERROR "GLM not found!")
endif()

#
# CREATE SHADER LIBRARY (headers generated into ${CMAKE_CURRENT_BINARY_DIR}/shader)
#
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shader)

set(SHADER_LIST
        phong_vert phong_frag
        shadow_vert shadow_frag
        color_vert color_frag
        diffuse_vert diffuse_frag
        texture_vert texture_frag
        convolution_vert convolution_frag
        blur_vert blur_frag
        bloom_frag
        skybox_vert skybox_frag
        normal_mapping_vert normal_mapping_frag
)

foreach(shader_name ${SHADER_LIST})
  set(HEADER_NAME "${shader_name}_glsl")
  set(HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/shader/${HEADER_NAME}.h")
  set(CPP_FILE    "${CMAKE_CURRENT_BINARY_DIR}/shader/${HEADER_NAME}.cpp")

  file(WRITE ${HEADER_FILE}
          "#ifndef ${HEADER_NAME}_H\n#define ${HEADER_NAME}_H\n\n#include <string>\n\nextern const std::string ${HEADER_NAME};\n\n#endif\n")

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shader/${shader_name}.glsl")
    file(READ  "${CMAKE_CURRENT_SOURCE_DIR}/shader/${shader_name}.glsl" SHADER_CONTENT)
    file(WRITE ${CPP_FILE}
            "#include \"${HEADER_NAME}.h\"\n\nconst std::string ${HEADER_NAME} = R\"(\n${SHADER_CONTENT}\n)\";\n")
    message(STATUS "Created shader from file: ${shader_name}")
  else()
    if(shader_name MATCHES ".*_vert")
      file(WRITE ${CPP_FILE}
              "#include \"${HEADER_NAME}.h\"\n\nconst std::string ${HEADER_NAME} = R\"(\n#version 330 core\nlayout(location = 0) in vec3 position;\nuniform mat4 model, view, projection;\nvoid main() { gl_Position = projection * view * model * vec4(position, 1.0); }\n)\";\n")
    else()
      file(WRITE ${CPP_FILE}
              "#include \"${HEADER_NAME}.h\"\n\nconst std::string ${HEADER_NAME} = R\"(\n#version 330 core\nout vec4 fragColor;\nvoid main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n)\";\n")
    endif()
    message(STATUS "Created default shader: ${shader_name}")
  endif()
endforeach()

file(GLOB SHADER_CPP_FILES "${CMAKE_CURRENT_BINARY_DIR}/shader/*.cpp")

# ВНИМАНИЕ: НЕ компилируем lamp.cpp в библиотеку shaders
add_library(shaders STATIC ${SHADER_CPP_FILES})
target_include_directories(shaders PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/shader)

#
# PPGSO library
#
# Создадим заголовок ppgso/ppgso.h при необходимости
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/ppgso")
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/ppgso/ppgso.h")
  file(WRITE "${CMAKE_SOURCE_DIR}/ppgso/ppgso.h"
          "#ifndef PPGSO_H\n#define PPGSO_H\n\n#include <GL/glew.h>\n#include <GLFW/glfw3.h>\n#include <glm/glm.hpp>\n\n#endif\n")
  message(STATUS "Created ppgso.h header")
endif()

if (ASSIMP_FOUND)
  message(STATUS "Using ASSIMP object loader")
  add_library(ppgso STATIC
          ppgso/Mesh_Assimp.cpp
          ppgso/tiny_obj_loader.cpp
          ppgso/shader.cpp
          ppgso/image.cpp
          ppgso/image_bmp.cpp
          ppgso/image_raw.cpp
          ppgso/texture.cpp
          ppgso/window.cpp
          # НЕ добавляем lamp.cpp/lamp.h в библиотеку ppgso
  )
else ()
  message(STATUS "Using TINY object loader")
  add_library(ppgso STATIC
          ppgso/Mesh_Tiny.cpp
          ppgso/tiny_obj_loader.cpp
          ppgso/shader.cpp
          ppgso/image.cpp
          ppgso/image_bmp.cpp
          ppgso/image_raw.cpp
          ppgso/texture.cpp
          ppgso/window.cpp
          # НЕ добавляем lamp.cpp/lamp.h в библиотеку ppgso
  )
endif ()

target_compile_definitions(ppgso PUBLIC -DGLM_FORCE_RADIANS -DGLEW_STATIC)

target_link_libraries(ppgso PUBLIC
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${OPENGL_LIBRARIES}
        stdc++fs
)
if (ASSIMP_FOUND)
  target_link_libraries(ppgso PUBLIC ${ASSIMP_LIBRARIES})
endif()

# КРИТИЧЕСКОЕ: добавить include каталога ppgso/ в саму библиотеку ppgso,
# чтобы инклюды вида <shader.h> находились корректно.
target_include_directories(ppgso PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/ppgso         # <-- ВАЖНО
        ${GLFW_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
)
if (ASSIMP_FOUND)
  target_include_directories(ppgso PUBLIC ${ASSIMP_INCLUDE_DIRS})
endif()

#
# TARGETS
#
add_executable(playground
        src/playground/playground.cpp
        src/playground/object.cpp
        src/playground/scene.cpp
        src/playground/camera.cpp
        data
        src/playground/light.cpp
        src/playground/objects/skybox.cpp
        src/playground/objects/plane.cpp
        src/playground/mainlight.cpp
        src/playground/mainlight.h

        src/playground/objects/building/building.cpp
        src/playground/objects/building/building.h
        src/playground/objects/building/balcony.cpp
        src/playground/objects/building/balcony.h
        src/playground/GenericModel.cpp


)

target_link_libraries(playground ppgso shaders)

target_include_directories(playground PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/ppgso   # для <shader.h>, <texture.h> и т.п.
        ${GLFW_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
)

add_custom_command(TARGET playground POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_CURRENT_BINARY_DIR}/
)

#
# INSTALLATION
#
install(TARGETS playground DESTINATION .)
install(TARGETS ppgso DESTINATION .)
install(TARGETS shaders DESTINATION .)
install(DIRECTORY data/ DESTINATION .)
