#include <cglm/cglm.h>

#include "terrain.h"
#include "perlin.h"

// array to store different terrain types
static const TerrainType terrain_types[TERRAIN_NUM_TYPES] = {
        {.color = (vec4s){0.2, 0.4, 0.75, 1.0},   .shininess = 150.0f, .height = TERRAIN_SEA_LEVEL},         // blue - water
        {.color = (vec4s){1.0, 1.0, 0.6, 1.0},    .shininess = 50.0f,  .height = TERRAIN_MAX_HEIGHT * 0.1},  // yellow - sand
        {.color = (vec4s){0.35, 0.65, 0.1, 1.0},  .shininess = 10.0f,  .height = TERRAIN_MAX_HEIGHT * 0.2},  // light green - thin grass
        {.color = (vec4s){0.3, 0.6, 0.1, 1.0},    .shininess = 10.0f,  .height = TERRAIN_MAX_HEIGHT * 0.3},  // green - grass
        {.color = (vec4s){0.25, 0.55, 0.1, 1.0},  .shininess = 10.0f,  .height = TERRAIN_MAX_HEIGHT * 0.4},  // dark green - thick grass
        {.color = (vec4s){0.35, 0.25, 0.25, 1.0}, .shininess = 25.0f,  .height = TERRAIN_MAX_HEIGHT * 0.7},  // grey - rock
        {.color = (vec4s){1.0, 1.0, 1.0, 1.0},    .shininess = 25.0f,  .height = TERRAIN_MAX_HEIGHT},        // white - snow
};

// initialize a single vertex values given x and z coordinates
static Vertex generate_vertex(const int x, const int z)
{
    // generate a height value between -TERRAIN_MAX_HEIGHT and TERRAIN_MAX_HEIGHT, with perlin noise
    const float height = ((fractal_noise(x / TERRAIN_SCALE, z / TERRAIN_SCALE, 1, 5) * 2) - 1) * TERRAIN_MAX_HEIGHT;

    // determine vertex color and shininess by the vertex height
    int i;
    for (i = 0; i < TERRAIN_NUM_TYPES; ++i) {
        if (height <= terrain_types[i].height) {
            break;
        }
    }

    // set vertex data
    const Vertex vertex = {
        .coords = {
            .x = x,
            .y = glm_max(height, TERRAIN_SEA_LEVEL),  // if the height is below sea level set it equal to it
            .z = z,
            .w = 1.0
        },

        .normal = (vec3s){0},

        .color     = terrain_types[i].color,
        .shininess = terrain_types[i].shininess
    };

    return vertex;
}

// fill the terrain array of vertices
static void fill_terrain_vertices(const int matrix_start_x, const int matrix_start_z, const int matrix_end_x, const int matrix_end_z,
                                  Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    const int world_start_x = position_x + (TERRAIN_SIZE / 2), world_start_z = position_z - (TERRAIN_SIZE / 2);
    int world_curr_x, world_curr_z;

    // move along z axis
    for (int j = matrix_start_z; j < matrix_end_z; ++j) {
        world_curr_z = - world_start_z - (j * TERRAIN_CHUNK_SIZE);

        // move along x axis
        for (int i = matrix_start_x; i < matrix_end_x; ++i) {
            world_curr_x = - world_start_x + (i * TERRAIN_CHUNK_SIZE);

            terrain_vertices[(j * TERRAIN_NUM_VERTICES_SIDE) + i] = generate_vertex(world_curr_x, world_curr_z);
       }
    }
}
