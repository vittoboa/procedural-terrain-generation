#ifndef PROCEDURAL_TERRAIN_GENERATION_LIGHT_H
#define PROCEDURAL_TERRAIN_GENERATION_LIGHT_H

#include <cglm/cglm.h>

typedef struct Light
{
   const vec4 ambient_colors;
   const vec4 diffuse_colors;
   const vec4 specular_colors;
   const vec4 coords;
} Light;

#endif //PROCEDURAL_TERRAIN_GENERATION_LIGHT_H
