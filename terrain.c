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

// fill the terrain array of indices
static void fill_terrain_indices(unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X])
{
    int current_pos;
    for (unsigned int j = 0; j < TERRAIN_NUM_VERTICES_SIDE - 1; ++j) {
        current_pos = j * TERRAIN_NUM_VERTICES_SIDE;

        // compute the indices of all vertices, two triangles at the time
        for (unsigned int i = 0; i < TERRAIN_NUM_INDICES_X; i += 6) {
            // bottom left triangle face
            terrain_indices[j][i    ] = current_pos + TERRAIN_NUM_VERTICES_SIDE;      // vertex below
            terrain_indices[j][i + 1] = current_pos;                                  // vertex
            terrain_indices[j][i + 2] = current_pos + TERRAIN_NUM_VERTICES_SIDE + 1;  // vertex below to the right

            // top right triangle face
            terrain_indices[j][i + 3] = current_pos;                                  // vertex
            terrain_indices[j][i + 4] = current_pos + 1;                              // vertex to the right
            terrain_indices[j][i + 5] = current_pos + TERRAIN_NUM_VERTICES_SIDE + 1;  // vertex below to the right

            ++current_pos;  // new current starting position every two triangles
        }
    }
}

// fill the terrain array of normals
static void fill_terrain_normals(const int matrix_start_x, const int matrix_start_z, const int matrix_end_x, const int matrix_end_z,
                                 unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                                 Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    // convert from number of squares to number of indices
    const int matrix_start_x_indices = matrix_start_x * NUM_TRIANGLES_IN_SQUARE * NUM_VERTICES_IN_TRIANGLE;
    const int matrix_end_x_indices   = matrix_end_x   * NUM_TRIANGLES_IN_SQUARE * NUM_VERTICES_IN_TRIANGLE;

    int v1_index, v2_index, v3_index;
    Vertex v1, v2, v3;
    vec3s edge1, edge2, normal;

    // compute the normals of all vertices, one triangle at the time
    for (int j = matrix_start_z; j < matrix_end_z - 1; ++j) {
        for (int i = matrix_start_x_indices; i < matrix_end_x_indices; ++i) {
            // get the indices of the triangle
            v1_index = terrain_indices[j][i];
            v2_index = terrain_indices[j][i + 1];
            v3_index = terrain_indices[j][i + 2];

            // get the vertices of the triangle
            v1 = terrain_vertices[v1_index];
            v2 = terrain_vertices[v2_index];
            v3 = terrain_vertices[v3_index];

            // get the vectors of two edges of the triangle
            glm_vec3_sub(v2.coords.raw, v1.coords.raw, edge1.raw);
            glm_vec3_sub(v3.coords.raw, v1.coords.raw, edge2.raw);

            // compute the normal
            glm_vec3_cross(edge1.raw, edge2.raw, normal.raw);

            // update the normal for all vertices
            glm_vec3_add(normal.raw, v1.normal.raw, terrain_vertices[v1_index].normal.raw);
            glm_vec3_add(normal.raw, v2.normal.raw, terrain_vertices[v2_index].normal.raw);
            glm_vec3_add(normal.raw, v3.normal.raw, terrain_vertices[v3_index].normal.raw);
        }
    }

    // normalize the vertices normals
    for (int j = matrix_start_z; j < matrix_end_z; ++j) {
        for (int i = matrix_start_x; i < matrix_end_x; ++i) {
            glm_normalize(terrain_vertices[i].normal.raw);
        }
    }
}
