#ifndef PROCEDURAL_TERRAIN_GENERATION_VERTEX_H
#define PROCEDURAL_TERRAIN_GENERATION_VERTEX_H

#include <cglm/types-struct.h>

typedef struct Vertex {
    vec4s coords;
    vec4s color;
    vec3s normal;
    float shininess;
} Vertex;

#endif //PROCEDURAL_TERRAIN_GENERATION_VERTEX_H
