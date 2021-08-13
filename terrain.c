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
static Vertex generate_vertex(const ivec3s pos)
{
    // generate a height value between -TERRAIN_MAX_HEIGHT and TERRAIN_MAX_HEIGHT, with perlin noise
    const float height = ((fractal_noise(pos.x / TERRAIN_SCALE, pos.z / TERRAIN_SCALE, 1, 5) * 2) - 1) * TERRAIN_MAX_HEIGHT;

    // determine vertex color and shininess by the vertex height
    size_t terrain_type_i = 0;
    for (; terrain_type_i < TERRAIN_NUM_TYPES; ++terrain_type_i) {
        if (height <= terrain_types[terrain_type_i].height) {
            break;
        }
    }

    // set vertex data
    const Vertex vertex = {
        .coords = {
            pos.x,
            glm_max(height, TERRAIN_SEA_LEVEL),  // if the height is below sea level set it equals to it
            pos.z,
            1.0
        },

        .normal = {0, 0, 0},

        .color     = terrain_types[terrain_type_i].color,
        .shininess = terrain_types[terrain_type_i].shininess
    };

    return vertex;
}

// fill the terrain array of vertices
static void fill_terrain_vertices(const ivec3s matrix_start, const ivec3s matrix_end,
                                  Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    const ivec3s world_start = { .x = position.x + (TERRAIN_SIZE / 2), .z = position.z - (TERRAIN_SIZE / 2) };

    for (size_t j = matrix_start.z; j < matrix_end.z; ++j) {
        for (size_t i = matrix_start.x; i < matrix_end.x; ++i) {
            const ivec3s world_pos = { .x = - world_start.x + (i * TERRAIN_CHUNK_SIZE),
                                       .z = - world_start.z - (j * TERRAIN_CHUNK_SIZE) };

            terrain_vertices[(j * TERRAIN_NUM_VERTICES_SIDE) + i] = generate_vertex(world_pos);
       }
    }
}

// update terrain vertices
void update_terrain_vertices(const ivec3s num_chunks, Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    const ivec3s sign = { .x = glm_sign(num_chunks.x), .z = glm_sign(num_chunks.z) };
    const ivec3s diff_shift = { .x = TERRAIN_NUM_VERTICES_SIDE - abs(num_chunks.x), .z = TERRAIN_NUM_VERTICES_SIDE - abs(num_chunks.z)};
    ivec3s start, end;

    // determine variables values to shift on x
    start.x = (num_chunks.x >= 0) ? (diff_shift.x - 1) : abs(num_chunks.x);
    end.x   = start.x - (diff_shift.x * sign.x);

    // determine variables values to shift on z
    start.z = (num_chunks.z >= 0) ? (diff_shift.z - 1) : abs(num_chunks.z);
    end.z   = start.z - (diff_shift.z * sign.z);

    // shift columns
    for (size_t j = start.z; j != end.z; j -= sign.z) {
        for (size_t i = 0; i < TERRAIN_NUM_VERTICES_SIDE; ++i) {
            const size_t pos_current = (j * TERRAIN_NUM_VERTICES_SIDE) + i;
            const size_t pos_new     = pos_current + (TERRAIN_NUM_VERTICES_SIDE * num_chunks.z);

            terrain_vertices[pos_new] = terrain_vertices[pos_current];
        }
    }
    // shift rows
    for (size_t j = 0; j < TERRAIN_NUM_VERTICES_SIDE; ++j) {
        for (size_t i = start.x; i != end.x; i -= sign.x) {
            const size_t pos_current = (j * TERRAIN_NUM_VERTICES_SIDE) + i;
            const size_t pos_new     = pos_current + num_chunks.x;

            terrain_vertices[pos_new] = terrain_vertices[pos_current];
        }
    }

    // generate new vertices on z
    start.x = 0;
    end.x   = TERRAIN_NUM_VERTICES_SIDE;
    start.z = (num_chunks.z >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks.z;
    end.z   = start.z + abs(num_chunks.z);
    fill_terrain_vertices(start, end, terrain_vertices);

    // generate new vertices on x
    start.x = (num_chunks.x >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks.x;
    end.x   = start.x + abs(num_chunks.x);
    start.z = end.z % TERRAIN_NUM_VERTICES_SIDE;
    end.z   = start.z + diff_shift.z;
    fill_terrain_vertices(start, end, terrain_vertices);
}

// fill the terrain array of indices
static void fill_terrain_indices(unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X])
{
    for (size_t j = 0; j < TERRAIN_NUM_VERTICES_SIDE - 1; ++j) {
        int current_pos = j * TERRAIN_NUM_VERTICES_SIDE;

        // compute the indices of all vertices, two triangles at the time
        for (size_t i = 0; i < TERRAIN_NUM_INDICES_X; i += 6) {
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
static void fill_terrain_normals(const ivec3s matrix_start, const ivec3s matrix_end,
                                 unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                                 Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    // convert from number of squares to number of indices
    const size_t matrix_start_x_indices = matrix_start.x * NUM_TRIANGLES_IN_SQUARE * NUM_VERTICES_IN_TRIANGLE;
    const size_t matrix_end_x_indices   = matrix_end.x   * NUM_TRIANGLES_IN_SQUARE * NUM_VERTICES_IN_TRIANGLE;

    // compute the normals of all vertices, one triangle at the time
    for (size_t j = matrix_start.z; j < matrix_end.z - 1; ++j) {
        for (size_t i = matrix_start_x_indices; i < matrix_end_x_indices; ++i) {
            vec3 edge1, edge2, normal;

            // get the indices of the triangle
            const size_t v1_index = terrain_indices[j][i    ];
            const size_t v2_index = terrain_indices[j][i + 1];
            const size_t v3_index = terrain_indices[j][i + 2];

            // get the vertices of the triangle
            Vertex v1 = terrain_vertices[v1_index];
            Vertex v2 = terrain_vertices[v2_index];
            Vertex v3 = terrain_vertices[v3_index];

            // get the vectors of two edges of the triangle
            glm_vec3_sub(v2.coords, v1.coords, edge1);
            glm_vec3_sub(v3.coords, v1.coords, edge2);

            // compute the normal
            glm_vec3_cross(edge1, edge2, normal);

            // update the normal for all vertices
            glm_vec3_add(normal, v1.normal, terrain_vertices[v1_index].normal);
            glm_vec3_add(normal, v2.normal, terrain_vertices[v2_index].normal);
            glm_vec3_add(normal, v3.normal, terrain_vertices[v3_index].normal);
        }
    }

    // normalize the vertices normals
    for (size_t j = matrix_start.z; j < matrix_end.z; ++j) {
        for (size_t i = matrix_start.x; i < matrix_end.x; ++i) {
            glm_normalize(terrain_vertices[(j * TERRAIN_NUM_VERTICES_SIDE) + i].normal);
        }
    }
}

// update the normals for the updated terrain
void update_terrain_normals(const ivec3s num_chunks,
                            unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                            Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE])
{
    ivec3s start, end;

    // update normals on z
    start.x = 0;
    end.x   = TERRAIN_NUM_VERTICES_SIDE;
    start.z = (num_chunks.z >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks.z;
    end.z   = start.z + abs(num_chunks.z);
    fill_terrain_normals(start, end, terrain_indices, terrain_vertices);

    // update normals on x
    start.x = (num_chunks.x >= 0) ? 0 : TERRAIN_NUM_VERTICES_SIDE + num_chunks.x;
    end.x   = start.x + abs(num_chunks.x);
    start.z = end.z % TERRAIN_NUM_VERTICES_SIDE;
    end.z   = start.z + (TERRAIN_NUM_VERTICES_SIDE - abs(num_chunks.z));
    fill_terrain_normals(start, end, terrain_indices, terrain_vertices);
}

// fill the terrain array of counts
static void fill_terrain_counts(int terrain_counts[TERRAIN_NUM_VERTICES_SIDE - 1])
{
    for (size_t i = 0; i < TERRAIN_NUM_VERTICES_SIDE - 1; ++i) {
        terrain_counts[i] = TERRAIN_NUM_INDICES_X;
    }
}

// fill the terrain array of offsets
static void fill_terrain_offsets(void* terrain_offsets[TERRAIN_NUM_VERTICES_SIDE - 1])
{
    for (size_t i = 0; i < TERRAIN_NUM_VERTICES_SIDE - 1; ++i) {
        terrain_offsets[i] = (GLvoid *) (TERRAIN_NUM_INDICES_X * i * sizeof(unsigned int));
    }
}

// procedurally generate terrain
void init_terrain(Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE],
                  unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X],
                  int terrain_counts[TERRAIN_NUM_VERTICES_SIDE - 1],
                  void* terrain_offsets[TERRAIN_NUM_VERTICES_SIDE - 1])
{
    fill_terrain_vertices((ivec3s) {0, 0, 0},
                          (ivec3s) {TERRAIN_NUM_VERTICES_SIDE, TERRAIN_NUM_VERTICES_SIDE, TERRAIN_NUM_VERTICES_SIDE},
                          terrain_vertices);
    fill_terrain_indices(terrain_indices);
    fill_terrain_normals((ivec3s) {0, 0, 0},
                         (ivec3s) {TERRAIN_NUM_VERTICES_SIDE, TERRAIN_NUM_VERTICES_SIDE, TERRAIN_NUM_VERTICES_SIDE},
                         terrain_indices, terrain_vertices); // needs to always be after filling terrain vertices and indices
    fill_terrain_counts(terrain_counts);
    fill_terrain_offsets(terrain_offsets);
}
