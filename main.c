// standard includes
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cglm/cglm.h>
#include <time.h>

// application specific includes
#include "terrain.h"
#include "shader.h"
#include "light.h"

// globals
#define NUM_DIFFERENT_MAPS 5000
#define MAX_VIEW_DISTANCE  400

// light properties
static const Light light0 =
{
    (vec4){0.1, 0.1, 0.1, 1.0},
    (vec4){0.9, 0.9, 0.9, 1.0},
    (vec4){0.5, 0.5, 0.5, 1.0},
    (vec4){10.0, 40.0, 10.0, 1.0}
};

// global ambient
static const vec4 global_ambient = (vec4)
{
    0.2, 0.2, 0.2, 1.0
};

// terrain data
static Vertex terrain_vertices[TERRAIN_NUM_VERTICES_SIDE * TERRAIN_NUM_VERTICES_SIDE];
static unsigned int terrain_indices[TERRAIN_NUM_VERTICES_SIDE - 1][TERRAIN_NUM_INDICES_X];
static int terrain_counts[TERRAIN_NUM_VERTICES_SIDE - 1];
static void* terrain_offsets[TERRAIN_NUM_VERTICES_SIDE - 1];

static mat4 model_view_matrix = GLM_MAT4_IDENTITY_INIT;
static mat4 projection_matrix = GLM_MAT4_IDENTITY_INIT;
static mat3 normal_matrix     = GLM_MAT3_IDENTITY_INIT;

// OpenGL global variables
int seed;
static unsigned int window_width = 1280, window_height = 720;
static GLint model_view_matrix_location, normal_matrix_location;

// OpenGL window resize routine
void resize(int new_width, int new_height)
{
    glViewport(0, 0, new_width, new_height);
    window_width  = new_width;
    window_height = new_height;
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // swap frame buffers
    glutSwapBuffers();
}

void init(void)
{
    // VBO ids
    enum buffer {TERRAIN_VERTICES, TERRAIN_INDICES};

    // set background color
    glClearColor(0.53, 0.81, 0.92, 1.0f);

    // allow removal of hidden faces
    glEnable(GL_CULL_FACE);

    // add depth
    glEnable(GL_DEPTH_TEST);

    // create shader program executable
    const GLuint program_id = glCreateProgram();
    const GLuint vertex_shader_id = setShader("vertex", "vertexShader.glsl");
    glAttachShader(program_id, vertex_shader_id);
    glLinkProgram(program_id);
    glUseProgram(program_id);

    // initialize terrain
    init_terrain(terrain_vertices, terrain_indices, terrain_counts, terrain_offsets);

    // create VAO and VBOs
    GLuint buffer[2], vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(2, buffer);

    // bind terrain data with vertex shader
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer[TERRAIN_VERTICES]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(terrain_vertices), terrain_vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[TERRAIN_INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(terrain_indices), terrain_indices, GL_STATIC_DRAW);
    // add coordinates
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), 0);
    glEnableVertexAttribArray(0);
    // add color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), (void*)(sizeof(terrain_vertices[0].coords)));
    glEnableVertexAttribArray(1);
    // add normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), (void*)(sizeof(terrain_vertices[0].coords)+sizeof(terrain_vertices[0].color)));
    glEnableVertexAttribArray(2);
    // add shininess
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), (void*)(sizeof(terrain_vertices[0].coords)+sizeof(terrain_vertices[0].color)+sizeof(terrain_vertices[0].normal)));
    glEnableVertexAttribArray(3);

    glm_perspective(glm_rad(50.0f), (float)window_width / (float)window_height, 1.0f, MAX_VIEW_DISTANCE, projection_matrix);

    // obtain matrices locations
    const GLint projection_matrix_location = glGetUniformLocation(program_id, "projection_matrix");
    model_view_matrix_location = glGetUniformLocation(program_id, "model_view_matrix");
    normal_matrix_location     = glGetUniformLocation(program_id, "normal_matrix");

    // obtain uniform locations and set values
    glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (GLfloat *) projection_matrix);

    // obtain light property uniform locations and set values
    glUniform4fv(glGetUniformLocation(program_id, "light0.ambient_colors"),  1, &light0.ambient_colors[0]);
    glUniform4fv(glGetUniformLocation(program_id, "light0.diffuse_colors"),  1, &light0.diffuse_colors[0]);
    glUniform4fv(glGetUniformLocation(program_id, "light0.specular_colors"), 1, &light0.specular_colors[0]);
    glUniform4fv(glGetUniformLocation(program_id, "light0.coords"),          1, &light0.coords[0]);

    // obtain global ambient uniform location and set value
    glUniform4fv(glGetUniformLocation(program_id, "global_ambient"), 1, &global_ambient[0]);
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);

    // set OpenGL version
    glutInitContextVersion(4, 6);
    // remove deprecated functions to make sure the program is compatible with future versions of OpenGL
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

    // create window
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Infinite Procedural Terrain Generator");
    glutReshapeFunc(resize);
    glutDisplayFunc(display);

    glewInit();

    srand(time(NULL));
    seed = rand() % NUM_DIFFERENT_MAPS;

    init();
    glutMainLoop();

    return 0;
}
