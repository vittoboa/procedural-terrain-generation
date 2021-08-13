#ifndef PROCEDURAL_TERRAIN_GENERATION_VERTEX_H
#define PROCEDURAL_TERRAIN_GENERATION_VERTEX_H

#include <cglm/types-struct.h>

typedef struct Vertex {
    vec4 coords;
    vec4s color;
    vec3 normal;
    float shininess;
} Vertex;

#endif //PROCEDURAL_TERRAIN_GENERATION_VERTEX_H
