#include <cglm/cglm.h>
#include <GL/glew.h>

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
    const int world_start_x = position.x + (TERRAIN_SIZE / 2), world_start_z = position.z - (TERRAIN_SIZE / 2);
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

// update terrain vertices
void update_terrain_vertices(const int num_chunks_x, const int num_chunks_z,
                             Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    const unsigned int diff_shift_x = TERRAIN_NUM_VERTICES_SIDE - abs(num_chunks_x), diff_shift_z = TERRAIN_NUM_VERTICES_SIDE - abs(num_chunks_z);
    const int sign_x = copysignf(1.0, num_chunks_x), sign_z = copysignf(1.0, num_chunks_z);
    const int shift_offset_z = (TERRAIN_NUM_VERTICES_SIDE * num_chunks_z);
    int pos_current, pos_new, start_x, end_x, start_z, end_z;

    // determine variables values to shift on x
    start_x = (num_chunks_x >= 0) ? (diff_shift_x - 1) : abs(num_chunks_x);
    end_x = start_x - (diff_shift_x * sign_x);

    // determine variables values to shift on z
    start_z = (num_chunks_z >= 0) ? (diff_shift_z - 1) : abs(num_chunks_z);
    end_z = start_z - (diff_shift_z * sign_z);

    // shift columns
    for (int j = start_z; j != end_z; j -= sign_z) {
        for (int i = 0; i < TERRAIN_NUM_VERTICES_SIDE; ++i) {
            pos_current = (j * TERRAIN_NUM_VERTICES_SIDE) + i;
            pos_new = pos_current + shift_offset_z;

            terrain_vertices[pos_new] = terrain_vertices[pos_current];
        }
    }
    // shift rows
    for (int j = 0; j < TERRAIN_NUM_VERTICES_SIDE; ++j) {
        for (int i = start_x; i != end_x; i -= sign_x) {
            pos_current = (j * TERRAIN_NUM_VERTICES_SIDE) + i;
            pos_new = pos_current + num_chunks_x;

            terrain_vertices[pos_new] = terrain_vertices[pos_current];
        }
    }

    // generate new vertices on z
    start_z = (num_chunks_z >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks_z;
    end_z = start_z + abs(num_chunks_z);
    fill_terrain_vertices(0, start_z, TERRAIN_NUM_VERTICES_SIDE, end_z, terrain_vertices);

    // generate new vertices on x
    start_x = (num_chunks_x >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks_x;
    end_x = start_x + abs(num_chunks_x);
    start_z = end_z % TERRAIN_NUM_VERTICES_SIDE;
    end_z = start_z + diff_shift_z;
    fill_terrain_vertices(start_x, start_z, end_x, end_z, terrain_vertices);
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

// update the normals for the updated terrain
void update_terrain_normals(const int num_chunks_x, const int num_chunks_z,
                            unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                            Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    // update normals on z
    int start_z = (num_chunks_z >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks_z;
    int end_z   = start_z + abs(num_chunks_z);
    fill_terrain_normals(0, start_z, TERRAIN_NUM_VERTICES_SIDE, end_z, terrain_indices, terrain_vertices);

    // update normals on x
    const int start_x = (num_chunks_x >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks_x;
    const int end_x   = start_x + abs(num_chunks_x);
    start_z = end_z % TERRAIN_NUM_VERTICES_SIDE;
    end_z   = start_z + (TERRAIN_NUM_VERTICES_SIDE - abs(num_chunks_z));
    fill_terrain_normals(start_x, start_z, end_x, end_z, terrain_indices, terrain_vertices);
}

// fill the terrain array of counts
static void fill_terrain_counts(int terrain_counts[TERRAIN_NUM_VERTICES_SIDE - 1])
{
    for (int i = 0; i < TERRAIN_NUM_VERTICES_SIDE - 1; ++i) {
        terrain_counts[i] = TERRAIN_NUM_INDICES_X;
    }
}

// fill the terrain array of offsets
static void fill_terrain_offsets(void* terrain_offsets[TERRAIN_NUM_VERTICES_SIDE - 1])
{
    for (int i = 0; i < TERRAIN_NUM_VERTICES_SIDE - 1; ++i) {
        terrain_offsets[i] = (GLvoid *) (TERRAIN_NUM_INDICES_X * i * sizeof(unsigned int));
    }
}

// procedurally generate terrain
void init_terrain(Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE],
                  unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                  int terrain_counts[TERRAIN_NUM_VERTICES_SIDE - 1],
                  void* terrain_offsets[TERRAIN_NUM_VERTICES_SIDE - 1])
{
    fill_terrain_vertices(0, 0, TERRAIN_NUM_VERTICES_SIDE, TERRAIN_NUM_VERTICES_SIDE, terrain_vertices);
    fill_terrain_indices(terrain_indices);
    fill_terrain_normals(0, 0, TERRAIN_NUM_VERTICES_SIDE, TERRAIN_NUM_VERTICES_SIDE, terrain_indices, terrain_vertices); // needs to always be after filling terrain vertices and indices
    fill_terrain_counts(terrain_counts);
    fill_terrain_offsets(terrain_offsets);
}
