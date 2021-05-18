#ifndef PROCEDURAL_TERRAIN_GENERATION_TERRAIN_H
#define PROCEDURAL_TERRAIN_GENERATION_TERRAIN_H

#include "vertex.h"

#define TERRAIN_MAX_HEIGHT 30
#define TERRAIN_SEA_LEVEL  0
#define TERRAIN_SCALE      65.5
#define TERRAIN_NUM_TYPES  7    // number of different types of terrains

typedef struct {
    const vec4s color;
    const float shininess;
    const float height;
} TerrainType;

#endif //PROCEDURAL_TERRAIN_GENERATION_TERRAIN_H
