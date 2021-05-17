// standard includes
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <time.h>

// application specific includes
#include "light.h"

// globals
#define NUM_DIFFERENT_MAPS 5000

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

// OpenGL global variables
int seed;
static unsigned int window_width = 1280, window_height = 720;

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
    // set background color
    glClearColor(0.53, 0.81, 0.92, 1.0f);

    // allow removal of hidden faces
    glEnable(GL_CULL_FACE);

    // add depth
    glEnable(GL_DEPTH_TEST);

    // create shader program executable
    const GLuint program_id = glCreateProgram();
    glLinkProgram(program_id);
    glUseProgram(program_id);

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
