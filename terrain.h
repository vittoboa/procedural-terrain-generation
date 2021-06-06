#ifndef PROCEDURAL_TERRAIN_GENERATION_TERRAIN_H
#define PROCEDURAL_TERRAIN_GENERATION_TERRAIN_H

#include "vertex.h"

#define NUM_VERTICES_IN_TRIANGLE  3    // number of vertices in a triangle
#define NUM_TRIANGLES_IN_SQUARE   2    // number of triangles to make a square
#define TERRAIN_MAX_HEIGHT 30
#define TERRAIN_SEA_LEVEL  0
#define TERRAIN_SCALE      65.5
#define TERRAIN_NUM_TYPES  7    // number of different types of terrains
#define TERRAIN_NUM_VERTICES_SIDE 500  // number of terrain's vertices in each axis
#define TERRAIN_NUM_INDICES_X (NUM_VERTICES_IN_TRIANGLE * NUM_TRIANGLES_IN_SQUARE * (TERRAIN_NUM_VERTICES_SIDE - 1))
#define TERRAIN_CHUNK_SIZE        2    // the size of each chunk, the distance between two vertices in the same axis
#define TERRAIN_SIZE (TERRAIN_NUM_VERTICES_SIDE * TERRAIN_CHUNK_SIZE)  // total size of terrain grid

extern vec3s position;  // current player position

typedef struct {
    const vec4s color;
    const float shininess;
    const float height;
} TerrainType;

void init_terrain(Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE],
                  unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                  int terrain_counts[TERRAIN_NUM_VERTICES_SIDE - 1],
                  void* terrain_offsets[TERRAIN_NUM_VERTICES_SIDE - 1]);

void update_terrain_vertices(const int num_chunks_x, const int num_chunks_z,
                             Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE]);

void update_terrain_normals(const int num_chunks_x, const int num_chunks_z,
                            unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                            Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE]);

#endif //PROCEDURAL_TERRAIN_GENERATION_TERRAIN_H
