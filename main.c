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
#define CAMERA_HEIGHT      15  // how much higher the camera is compared to the maximum height of the mountains
#define UPDATE_THRESHOLD   5   // distance between terrain updates
#define MOVEMENT_SPEED     2   // how quickly the player can move
static float angle_rad_y = 0.0;  // angle to rotate scene
vec3s position = {0.0, -TERRAIN_MAX_HEIGHT - CAMERA_HEIGHT, 0.0};  // player current position

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // generate new model view matrix
    glm_mat4_identity(model_view_matrix);
    // rotate around the center at the player angle
    glm_rotate_y(model_view_matrix, angle_rad_y, model_view_matrix);
    // move to the player position
    glm_translate(model_view_matrix, position.raw);
    // update model view matrix
    glUniformMatrix4fv(model_view_matrix_location, 1, GL_FALSE, (GLfloat *)model_view_matrix);

    /* Update normal matrix */
    mat3 TMP;
    glm_mat4_pick3(model_view_matrix, TMP);
    glm_mat3_inv(TMP, normal_matrix);
    glm_mat3_transpose(normal_matrix);
    glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, (GLfloat *)normal_matrix);

    /* Draw terrain */
    glMultiDrawElements(GL_TRIANGLE_STRIP, terrain_counts, GL_UNSIGNED_INT, (const void **)terrain_offsets, TERRAIN_NUM_VERTICES_SIDE - 1);

    /* Update terrain */
    static enum update_steps { POSITION, VERTICES, NORMALS, VBO } step;
    static vec3s position_last_update;  // player position at the time of the last terrain update
    const bool should_update_x = abs((int)(position.x - position_last_update.x)) >= UPDATE_THRESHOLD;
    const bool should_update_z = abs((int)(position.z - position_last_update.z)) >= UPDATE_THRESHOLD;

    if (step || should_update_x || should_update_z) {
        static ivec3s num_chunks;  // number of chunks to update

        switch (step) {
            case POSITION: {
                // determine number of chunks to generate on the x and z axis
                num_chunks = (ivec3s) { .x = round(((position.x - position_last_update.x) / TERRAIN_CHUNK_SIZE)),
                                        .z = round(((position_last_update.z - position.z) / TERRAIN_CHUNK_SIZE)) };

                // update variable to track user position at the time of the last update
                glm_vec3_copy(position.raw, position_last_update.raw);

                ++step;
                break;
            }
            case VERTICES: {
                // update the new terrain at the current location
                update_terrain_vertices(num_chunks, terrain_vertices);

                ++step;
                break;
            }
            case NORMALS: {
                // update the vertices normals
                update_terrain_normals(num_chunks, terrain_indices, terrain_vertices);

                ++step;
                break;
            }
            case VBO: {
                // update the vertices in the vbo
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(terrain_vertices), terrain_vertices);

                step = 0;
                break;
            }
            default: {
                break;
            }
        }
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), 0);
    glEnableVertexAttribArray(0);
    // add color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), (void*)(sizeof(terrain_vertices[0].coords)));
    glEnableVertexAttribArray(1);
    // add normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), (void*)(sizeof(terrain_vertices[0].coords)+sizeof(terrain_vertices[0].color)));
    glEnableVertexAttribArray(2);
    // add shininess
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(terrain_vertices[0]), (void*)(sizeof(terrain_vertices[0].coords)+sizeof(terrain_vertices[0].color)+sizeof(terrain_vertices[0].normal)));
    glEnableVertexAttribArray(3);

    glm_perspective(glm_rad(50.0f), (float)window_width / window_height, 1.0f, 600.0f, projection_matrix);

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
            position.z += MOVEMENT_SPEED * sin(angle_rad_y + GLM_PI_2);
            position.x += MOVEMENT_SPEED * cos(angle_rad_y + GLM_PI_2);
            glutPostRedisplay();
            break;
        }
        case 'S':  // move backward
        case 's': {
            position.z -= MOVEMENT_SPEED * sin(angle_rad_y + GLM_PI_2);
            position.x -= MOVEMENT_SPEED * cos(angle_rad_y + GLM_PI_2);
            glutPostRedisplay();
            break;
        }
        case 'A':  // rotate left
        case 'a': {
            angle_rad_y -= glm_rad(1);
            glutPostRedisplay();
            break;
        }
        case 'D':  // rotate right
        case 'd': {
            angle_rad_y += glm_rad(1);
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

    // set the seed which determines the map to generate
    srand(time(NULL));
    const int num_different_maps = 5000;
    seed = rand() % num_different_maps;

    init();
    glutMainLoop();

    return 0;
}
