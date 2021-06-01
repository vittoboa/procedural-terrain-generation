// standard includes
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <time.h>

// application specific includes
#include "terrain.h"
#include "shader.h"
#include "light.h"

// globals
#define NUM_DIFFERENT_MAPS 5000
#define MAX_VIEW_DISTANCE  350
#define UPDATE_THRESHOLD   20  // distance between terrain updates
#define CAMERA_HEIGHT      15  // how much higher the camera is compared to the maximum height of the mountains
#define MOVEMENT_SPEED     2   // how quickly the player can move
static float angle_y = 0.0;  // angle to rotate scene
static float last_update_pos_x = 0.0, last_update_pos_z = 0.0;  // player position at the time of the last terrain update
float position_x = 0.0, position_y = -TERRAIN_MAX_HEIGHT - CAMERA_HEIGHT, position_z = 0.0;  // player current position

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
static GLsizei window_width = 1280, window_height = 720;
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
    mat3 TMP;
    const bool should_update_x = abs((int)(position_x - last_update_pos_x)) >= UPDATE_THRESHOLD;
    const bool should_update_z = abs((int)(position_z - last_update_pos_z)) >= UPDATE_THRESHOLD;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // generate new model view matrix
    glm_mat4_identity(model_view_matrix);
    // rotate around the center at the player angle
    glm_rotate(model_view_matrix, glm_rad(angle_y), (vec3){0.0, 1.0, 0.0});
    // move to the player position
    glm_translate(model_view_matrix, (vec3){position_x, 0.0, 0.0});
    glm_translate(model_view_matrix, (vec3){0.0, position_y, 0.0});
    glm_translate(model_view_matrix, (vec3){0.0, 0.0, position_z});
    // update model view matrix
    glUniformMatrix4fv(model_view_matrix_location, 1, GL_FALSE, (GLfloat *)model_view_matrix);

    // calculate and update normal matrix
    glm_mat4_pick3(model_view_matrix, TMP);
    glm_mat3_inv(TMP, normal_matrix);
    glm_mat3_transpose(normal_matrix);
    glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, (GLfloat *)normal_matrix);

    // draw terrain
    glMultiDrawElements(GL_TRIANGLE_STRIP, terrain_counts, GL_UNSIGNED_INT, (const void **)terrain_offsets, TERRAIN_NUM_VERTICES_SIDE - 1);

    // update terrain
    if (should_update_x || should_update_z) {
        // determine number of chunks to generate on the x and z axis
        const int num_chunks_x = round(((position_x - last_update_pos_x) / TERRAIN_CHUNK_SIZE));
        const int num_chunks_z = round(((last_update_pos_z - position_z) / TERRAIN_CHUNK_SIZE));

        // update the new terrain at the current location
        update_terrain_vertices(num_chunks_x, num_chunks_z, terrain_vertices);

        // update the vertices normals
        update_terrain_normals(num_chunks_x, num_chunks_z, terrain_indices, terrain_vertices);

        // update the vertices in the vbo
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(terrain_vertices), terrain_vertices);

        // update variables to track user position at the time of the last update
        last_update_pos_x = position_x;
        last_update_pos_z = position_z;
    }

    // swap frame buffers
    glutSwapBuffers();
}

void init(void)
{
    // VBO ids
    enum buffer {TERRAIN_VERTICES, TERRAIN_INDICES};

    // light properties
    const Light light0 =
    {
        {0.1, 0.1, 0.1, 1.0},
        {0.9, 0.9, 0.9, 1.0},
        {0.5, 0.5, 0.5, 1.0},
        {10.0, 40.0, 10.0, 1.0}
    };

    // global ambient light
    const vec4 global_ambient = {0.2, 0.2, 0.2, 1.0};

    // set background color
    glClearColor(0.53, 0.81, 0.92, 1.0f);

    // allow removal of hidden faces
    glEnable(GL_CULL_FACE);

    // add depth
    glEnable(GL_DEPTH_TEST);

    // create shader program executable
    const GLuint program_id = glCreateProgram();
    const GLuint vertex_shader_id   = setShader("vertex",   "vertexShader.glsl");
    const GLuint fragment_shader_id = setShader("fragment", "fragmentShader.glsl");
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
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

// keyboard input processing routine
void keyInput(unsigned char key, int x, int y)
{
    switch(key) {
        case 27:
            exit(0);
        case 'W':  // move forward
        case 'w': {
            position_z += MOVEMENT_SPEED * sin(glm_rad(angle_y + 90));
            position_x += MOVEMENT_SPEED * cos(glm_rad(angle_y + 90));
            glutPostRedisplay();
            break;
        }
        case 'S':  // move backward
        case 's': {
            position_z -= MOVEMENT_SPEED * sin(glm_rad(angle_y + 90));
            position_x -= MOVEMENT_SPEED * cos(glm_rad(angle_y + 90));
            glutPostRedisplay();
            break;
        }
        case 'A':  // rotate left
        case 'a': {
            --angle_y;
            glutPostRedisplay();
            break;
        }
        case 'D':  // rotate right
        case 'd': {
            ++angle_y;
            glutPostRedisplay();
            break;
        }
        default: {
             break;
        }
    }
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
    glutKeyboardFunc(keyInput);

    glewInit();

    srand(time(NULL));
    seed = rand() % NUM_DIFFERENT_MAPS;

    init();
    glutMainLoop();

    return 0;
}
